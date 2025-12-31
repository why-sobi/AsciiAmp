#include <termviz.hpp>
#include <image.hpp>
#include <music.hpp>
#include <kiss_fft.h>

inline extern constexpr int FULL_WINDOW_WIDTH = 261;
inline extern constexpr int IMAGE_H = 26;
inline extern constexpr int IMAGE_W = 72;
inline extern constexpr int TITLE_H = 5;

inline extern constexpr char BOLD[] = "\033[1m";
inline extern constexpr char DIM_BOLD[] = "\033[2m";
inline extern constexpr char ITALIC[] = "\033[3m";
inline extern constexpr char UNDERLINE[] = "\033[4m";


template <typename T>
T random_gen(const T min, const T max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<T> dist(min, max);

    return dist(gen);
} 

std::string toUpper(const std::string& str) {
    std::string res = str.substr(0); // copying complete string
    for (char& ch : res) ch = std::toupper(ch);
    return res;
}

void printCover(const Music& music) { // wrapping in function release memory asap
    int window_width = IMAGE_W, window_height = IMAGE_H; // as height is double than width in terminal
    termviz::Window window(1, 1, window_width, window_height, "BOX");

    Image img(music.coverArt);
    img.downScale(window.get_w(), window.get_h(), SAMPLE::BOX);
    
    auto [characters, colors] = img.toAscii();

    termviz::Visualizer::Plots::draw_frame(window, characters, colors);
    window.render();


    // termviz::Window win2(window_width + 1, 1, window_width, window_height, "BILINEAR INTERPOLATION");
    // Image img2(music.coverArt);
    // img2.downScale(window.get_w(), window.get_h(), SAMPLE::BILINEAR_INTERPOLATION);
    // auto [char2, colors2] = img2.toAscii();

    // termviz::Visualizer::Plots::draw_frame(win2, char2, colors2);
    // win2.render();
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
    std::string result = style + str + "\033[0m";
    return result;
}

int getPadding(const std::string& str, int window_w) {
    int padding = ((window_w - str.length()) / 2);
    if (padding < 0) padding = 0;

    return padding;
}

std::vector<int> groupIntoNBars(kiss_fft_cpx* fft_output, int n_bars, int max_height) {
    std::vector<int> bars(n_bars);
    
    // Total bins we care about (first half of the 1024 FFT)
    const int total_bins = 512;

 for (int i = 0; i < n_bars; i++) {
    // 1. NON-LINEAR BUCKETING
    // We use a logarithmic distribution so that each bar covers a 
    // sensible range of frequencies (more bins for high bars).
    float low_pct = (float)i / n_bars;
    float high_pct = (float)(i + 1) / n_bars;

    // Log-space mapping: This ensures high bars grab more bins to stay "alive"
    int start_bin = (int)(std::pow(total_bins, low_pct));
    int end_bin = (int)(std::pow(total_bins, high_pct));

    // Safety check: ensure at least one bin per bar
    if (end_bin <= start_bin) end_bin = start_bin + 1;
    if (end_bin > total_bins) end_bin = total_bins;

    // 2. PEAK DETECTION (Instead of Average)
    // Averaging "kills" the height of bars with many bins. 
    // Picking the Max bin makes the visualizer much more responsive.
    float max_mag = 0.0f;
    for (int b = start_bin; b < end_bin; b++) {
        float r = fft_output[b].r;
        float im = fft_output[b].i;
        float mag = std::sqrt(r * r + im * im);
        if (mag > max_mag) max_mag = mag;
    }

    // 3. THE "ZERO KILLER": DECIBEL SCALING
    // Raw FFT data is linear. Human hearing is logarithmic (Decibels).
    // This turns tiny values like 0.001 into visible movement.
    float min_db = -60.0f; // Anything quieter than this is "zero"
    float db = 20.0f * std::log10(max_mag + 1e-7f); // 1e-7 prevents log(0) error
    
    // Normalize dB to 0.0 -> 1.0 range
    float intensity = (db - min_db) / (-min_db);
    if (intensity < 0) intensity = 0;

    // 4. EQUAL LOUDNESS BOOST
    // High frequencies need a massive artificial boost to look "balanced" with bass
    float boost = 1.0f + (low_pct * 2.5f);
    float final_val = intensity * boost;

    // 5. SMOOTHING (Interpolation)
    // If the new value is higher, snap to it. If lower, drift down slowly.
    // This creates the "gravity" effect so bars don't flicker to zero.
    float current_height = (float)bars[i];
    float target_height = final_val * max_height;
    
    float smoothing = (target_height > current_height) ? 0.7f : 0.2f;
    bars[i] = (int)(current_height + (target_height - current_height) * smoothing);

    // Final Clamp
    bars[i] = std::clamp(bars[i], 1, max_height);
}

    return bars;
}

std::vector<int> getVizBars(size_t playbackIndex, const std::vector<float>& monoSamples, kiss_fft_cfg cfg, int nBars, int winHeight) {
    kiss_fft_cpx in[1024]; // real and imaginary numbers 
    kiss_fft_cpx out[1024]; // real and imaginary numbers 

    // 1. Fill the 'in' buffer with a "window" of the song
    for (int i = 0; i < 1024; i++) {
        size_t sampleIdx = playbackIndex + i;
        
        if (sampleIdx < monoSamples.size()) {
            // Hann Window here to prevent "flicker"
            float window = 0.5f * (1.0f - cos(2.0f * M_PI * i / (1024 - 1)));
            in[i].r = monoSamples[sampleIdx] * window;
            in[i].i = 0;
        } else {
            in[i].r = 0; in[i].i = 0; // Padding if song ends
        }
    }

    // 2. Perform the Math
    kiss_fft(cfg, in, out);

    return groupIntoNBars(out, nBars, winHeight);
}

