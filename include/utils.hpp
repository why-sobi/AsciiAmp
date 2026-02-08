#include <image.hpp>
#include <music.hpp>
#include <playback.hpp>

#include <echo.hpp>
#include <kiss_fft.h>

#include <vector>
#include <chrono>
#include <thread>


#ifdef _WIN32
    #include <conio.h>  // For Windows kbhit() and getch()
#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <stdio.h>

    // Linux implementation of kbhit()
    int kbhit() {
        struct termios oldt, newt;
        int ch;
        int oldf;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        if(ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    }

    // Linux implementation of getch()
    int _getch() {
        struct termios oldt, newt;
        int ch;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }
#endif


inline extern constexpr int FULL_WINDOW_WIDTH = 261;
inline extern constexpr int IMAGE_H = 26;
inline extern constexpr int IMAGE_W = 72;
inline extern constexpr int TITLE_H = 5;
inline extern constexpr int PLAYBACK_H = 7;


inline extern constexpr char NORMAL[] = "\033[0m";
inline extern constexpr char BOLD[] = "\033[1m";
inline extern constexpr char DIM_BOLD[] = "\033[2m";
inline extern constexpr char ITALIC[] = "\033[3m";
inline extern constexpr char UNDERLINE[] = "\033[4m";

constexpr float REFERENCE = 40.0f; // what's considered loud 

template <typename T>
T random_gen(const T min, const T max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<T> dist(min, max);

    return dist(gen);
} 

void printCover(const Music& music) { // wrapping in function release memory asap
    int window_width = IMAGE_W, window_height = IMAGE_H; // as height is double than width in terminal
    echo::Window window(1, 1, window_width, window_height, "BOX");

    Image img(music.coverArt);
    img.downScale(window.get_w(), window.get_h(), SAMPLE::BOX);
    
    auto [characters, colors] = img.toAscii();

    echo::Visualizer::Plots::draw_frame(window, characters, colors);
    window.render();
} 

void printCover(const std::string& path = "C:/Users/shadows box/Downloads/2.jpeg") { // wrapping in function release memory asap
    int window_width = 75, window_height = 25; // as height is double than width in terminal
    echo::Window window(1, 1, window_width, window_height);

    Image img(path);
    img.downScale(window.get_w(), window.get_h());

    auto [characters, colors] = img.toAscii();
    echo::Visualizer::Plots::draw_frame(window, characters, colors);
    window.render();
}

void draw_random_bars(echo::Window& win) {
    const int max_bar_width = 7;
    int get_max_bars = echo::Visualizer::Plots::getMaxBars(win, max_bar_width);

    std::vector<int> heights(get_max_bars);
    std::vector<echo::COLOR> colors(get_max_bars, echo::COLOR::BLUE);
    for (int& val : heights) { val = random_gen<int>(0, win.get_h()); }

    echo::Visualizer::Plots::draw_bars(win, heights, max_bar_width, colors);
    win.render(true);
}

std::string format(const std::string& str, const char style[] = "\033[0m") {
    std::string style_view(style);
    std::string result;
    result.reserve(style_view.length() + str.length() + 5); // accomodate for everything at once

    result += style;
    result += str;
    result += "\033[0m";

    return result;
}

std::string toUpper(std::string str) {
    for (char& ch : str) ch = std::toupper(ch);
    return str;
}

int getPadding(const std::string& str, int window_w) {
    int padding = ((window_w - str.length()) / 2);
    if (padding < 0) padding = 0;

    return padding;
}

std::vector<float> getFFTwindow(const Playback& playbackInfo, size_t window_size = 1024) { // window to send to kissfft 
    std::vector<float> window(window_size, 0.0f);
    size_t currPos = playbackInfo.playhead.load();

    for (size_t i = 0; i < window_size; i++) {
        size_t idx = currPos + i;
        if (idx < playbackInfo.samples->size()) { window[i] = playbackInfo.samples->at(idx); }
        else break;
    }

    return window;
}

// NOTE: cfg is a pointer hence by value
// also returns mirrored maxBars (calculates fft for maxBars / 2) then mirrors it
std::vector<int> getNbars(const Playback& playbackInfo, kiss_fft_cfg cfg, int maxBars, int maxHeight) {
    std::vector<float> window = getFFTwindow(playbackInfo);
    size_t nfft = window.size();

    // setup input and output (cpx is a struct with .r and .i for real and imaginary part respectively)
    std::vector<kiss_fft_cpx> in(nfft);
    std::vector<kiss_fft_cpx> out(nfft);

    for (int i = 0; i < nfft; ++i) {
        in[i].r = window[i];
        in[i].i = 0.0f; // Audio samples have no imaginary part
    }

    kiss_fft(cfg, in.data(), out.data());

    // Calculate Magnitudes
    // The result is symmetrical, so we only need the first half (0 to nfft/2)
    std::vector<float> magnitudes;
    magnitudes.reserve(nfft / 2);

    for (int i = 0; i < nfft / 2; ++i) {
        // Pythagorean theorem to get the "loudness" of this frequency
        float mag = std::sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
        magnitudes.push_back(mag);
    }

    // Now we can convert the magnitudes to bars
    // for one side we calculate

    int half_size = (maxBars & 1) ? (maxBars / 2) + 1 : maxBars / 2;
    std::vector<int> bars(half_size, 0);
    int numBins = (int)magnitudes.size();

    for (int i = 0; i < half_size; ++i) {
        // 1. Logarithmic Binning (Frequency Spacing)
        float startRel = (float)i / half_size;
        float endRel = (float)(i + 1) / half_size;
        
        int startBin = (int)(pow(startRel, 1.5f) * numBins);
        int endBin = (int)(pow(endRel, 1.5f) * numBins);
        if (endBin <= startBin) endBin = startBin + 1;

        float sum = 0;
        for (int j = startBin; j < endBin && j < numBins; ++j) {
            sum += magnitudes[j];
        }
        float avg = sum / (endBin - startBin);

        float intensity = 0;
        if (avg > 0) {
            // 1. Normalize: Get a ratio between 0.0 and 1.0
            float ratio = avg / REFERENCE;
            
            // 2. Log Scale: This squashes the range so it's not "all or nothing"
            // Use log2 or a smaller multiplier than 20 to keep it chill
            intensity = 20 * log10(1.0f + ratio); 
        }

        // 3. Scale to maxHeight
        // This ensures 'intensity' acts as a percentage (0.0 to 1.0) of your height
        int h = (int)(intensity * maxHeight) + 1; // we scaled everything by +1

        bars[i] = std::clamp(h, 0, maxHeight);
    }

    std::vector<int> fullBars;
    fullBars.reserve(maxBars);

    for (int i = half_size - 1; i >= (maxBars & 1) ; i--) {
        fullBars.push_back(bars[i]);
    }

    // the remaining half
    fullBars.insert(fullBars.end(), bars.begin(), bars.end());
    return fullBars;
}

int64_t timestampToSeconds(const std::string& timestamp) {
    int minutes = 0;
    int seconds = 0;
    char colon;

    std::stringstream ss(timestamp);
    
    // Parse the string (Expected format "MM:SS")
    if (ss >> minutes >> colon >> seconds) {
        return (std::chrono::minutes(minutes) + std::chrono::seconds(seconds)).count();
    }

    // Return 0 if parsing fails
    return std::chrono::seconds(0).count();
}

void runTimestamp(echo::Window& playback, const Music& music, const Playback& playbackInfo, int playback_width, int bar_width, int starting_col) {
    namespace tv = echo;
    namespace Viz = echo::Visualizer::Plots;
    
    int total_duration = timestampToSeconds(music.duration);

    while (playbackInfo.isPlaying) { 
        if (playbackInfo.pause.load()) continue;
        auto now = std::chrono::steady_clock::now();
        auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - playbackInfo.startTime.load()).count();
        if (total_seconds > total_duration) total_seconds = total_duration;

        // 1. Fixed-width Time Formatting
        char current_time[16];
        snprintf(current_time, sizeof(current_time), "%d:%02d", (int)(total_seconds / 60), (int)(total_seconds % 60));
        
        // 2. Progress Bar Math
        int played_amount = (total_duration > 0) ? (int)(((float)total_seconds / total_duration) * bar_width) : 0;

        std::string bar = "[" + std::string(played_amount, '#') + std::string(bar_width - played_amount, '-') + "]";
        std::string progress_line = std::string(current_time) + bar + music.duration;


        // 3. Dynamic Technical Status Line
        // Uses the new bitrate, sample_rate, and channels data
        std::string chan_text = (music.channels == 2) ? "Stereo" : (music.channels == 1 ? "Mono" : "Multi");
        char tech_buf[128];
        snprintf(tech_buf, sizeof(tech_buf), "%s kbps | %.1f kHz | %s", 
                 music.bitrate.c_str(), music.sample_rate / 1000.0f, chan_text.c_str());
        
        std::string tech_status = tech_buf;
        int tech_padding = (playback_width - tech_status.length()) / 2;

        // 4. Keyboard Shortcuts Legend
        std::string legend = " [P] Pause/Play    [B] Back    [Q] Quit    [<-/->] Restart/Next";
        
        // --- RENDERING ---
        // Top Row: Real-time File Stats
        playback.print(playback.get_h() / 2 - 2, starting_col, BOLD + tech_status + NORMAL);
        
        // Middle Row: Blue Progress Bar
        playback.print(playback.get_h() / 2, starting_col, BOLD + progress_line + NORMAL, tv::COLOR(tv::COLOR::BLUE));

        // Bottom Row: Controls
        playback.print(playback.get_h() / 2 + 2, getPadding(legend, playback.get_w()), BOLD + legend + NORMAL);

        playback.render(false);
        std::this_thread::sleep_for(1s);
    }
}

inline void screenInit(const Music& music, echo::Window& title) { // display everthing at the start of the music
    // print and setup everything else
    printCover(music);
    title.clean_buffer();
    title.print(0, getPadding(music.title, title.get_w()), format(toUpper(music.title), BOLD));
    title.print(1, getPadding(music.artist, title.get_w()), format(toUpper(music.artist), UNDERLINE));
    title.print(2, getPadding(music.album, title.get_w()), format(toUpper(music.album)));
    title.render(true); 
}

// takes action based on keyboard input and return the key pressed (which is used for some controls in main)
char controller(Playback& playbackInfo, ma_device *pDevice) { 
    if (kbhit()) {
        int code = _getch();

        // Cross-platform Arrow Key Handling
        // Windows: Starts with 0 or 224
        // Linux: Starts with Escape (27), then '[', then A, B, C, or D
        #ifdef _WIN32
        if (code == 0 || code == 224) {
            int arrow = _getch();
            switch (arrow) {
                case 72: return 'U'; // UP
                case 80: return 'D'; // DOWN
                case 75: 
                    playbackInfo.playhead.store(0);
                    playbackInfo.startTime.store(std::chrono::steady_clock::now());
                    break; 
                case 77: 
                    playbackInfo.isPlaying = false; // Skip to next song 
                    break;
            }
        }
        #else
        if (code == 27) { // Escape sequence
            _getch(); // Skip '['
            int arrow = _getch();
            switch (arrow) {
                case 'A': return 'U'; // UP
                case 'B': return 'D'; // DOWN
                case 'D': 
                    playbackInfo.playhead.store(0);
                    playbackInfo.startTime.store(std::chrono::steady_clock::now());
                    break;                
                case 'C':
                    playbackInfo.isPlaying = false; // Skip to next song 
                    break;
            }
        }
        #endif

        // Standard Actions
        if (code == 'q' || code == 'Q') {
            playbackInfo.isPlaying = false; 
            ma_device_stop(pDevice); 
            ma_device_uninit(pDevice);
            echo::reset_cursor(); 
            return 'q';
        } 
        
        // Navigation Logic (Normalized arrow returns)
        char action = (char)code;
        // If we returned U, D, L, R from the arrow logic above, handle it here
        // (Simplified for brevity, apply your playbackInfo logic here)
        
        if (code == 'p' || code == 'P' || code == ' ') {
            bool paused = playbackInfo.pause.load();
            if (!paused) {
                playbackInfo.pausedAt = std::chrono::steady_clock::now();
            } else {
                auto now = std::chrono::steady_clock::now();
                auto pause_duration = now - playbackInfo.pausedAt;
                playbackInfo.startTime.store(playbackInfo.startTime.load() + 
                    std::chrono::duration_cast<std::chrono::milliseconds>(pause_duration));
            }
            playbackInfo.pause.store(!paused);
        }
        return code;
    }
    return '\0';
}