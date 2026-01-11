#include <image.hpp>
#include <music.hpp>
#include <playback.hpp>

#include <termviz.hpp>
#include <kiss_fft.h>

#include <vector>

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
    termviz::Window window(1, 1, window_width, window_height, "BOX");

    Image img(music.coverArt);
    img.downScale(window.get_w(), window.get_h(), SAMPLE::BOX);
    
    auto [characters, colors] = img.toAscii();

    termviz::Visualizer::Plots::draw_frame(window, characters, colors);
    window.render();
} 

void printCover(const std::string& path = "C:/Users/shadows box/Downloads/2.jpeg") { // wrapping in function release memory asap
    int window_width = 75, window_height = 25; // as height is double than width in terminal
    termviz::Window window(1, 1, window_width, window_height);

    Image img(path);
    img.downScale(window.get_w(), window.get_h());

    auto [characters, colors] = img.toAscii();
    termviz::Visualizer::Plots::draw_frame(window, characters, colors);
    window.render();
}

void draw_random_bars(termviz::Window& win) {
    const int max_bar_width = 7;
    int get_max_bars = termviz::Visualizer::Plots::getMaxBars(win, max_bar_width);

    std::vector<int> heights(get_max_bars);
    std::vector<termviz::COLOR> colors(get_max_bars, termviz::COLOR::BLUE);
    for (int& val : heights) { val = random_gen<int>(0, win.get_h()); }

    termviz::Visualizer::Plots::draw_bars(win, heights, max_bar_width, colors);
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