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

// std::vector<float> getFFTwindow(const Playback& playbackInfo, size_t window_size = 1024) { // window to send to kissfft 
//     std::vector<float> window(window_size, 0.0f);
//     size_t currPos = playbackInfo.playhead.load();

//     for (size_t i = 0; i < window_size; i++) {
//         size_t idx = currPos + i;
//         if (idx < playbackInfo.samples->size()) { window[i] = playbackInfo.samples->at(idx); }
//         else break;
//     }

//     return window;
// }

// std::vector<int> getNbars(const Playback& playbackInfo, int nbars, int maxHeight) {
//     std::vector<float> window = getFFTwindow(playbackInfo);
//     size_t nfft = window.size();

//     // now we setup kissfft
//     kiss_fft_cfg cfg = kiss_fit_alloc(int(nfft), 0, nullptr, nullptr);

//     // setup input and output (cpx is a struct with .r and .i for real and imaginary part respectively)
//     std::vector<kiss_fft_cpx> in(nfft);
//     std::vector<kiss_fft_cpx> out(nfft);

//     for (int i = 0; i < nfft; ++i) {
//         in[i].r = window[i];
//         in[i].i = 0.0f; // Audio samples have no imaginary part
//     }

//     kiss_fft(cfg, in.data(), out.data());

//     // Calculate Magnitudes
//     // The result is symmetrical, so we only need the first half (0 to nfft/2)
//     std::vector<float> magnitudes;
//     magnitudes.reserve(nfft / 2);

//     for (int i = 0; i < nfft / 2; ++i) {
//         // Pythagorean theorem to get the "loudness" of this frequency
//         float mag = std::sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
//         magnitudes.push_back(mag);
//     }

//     // Clean up the plan to avoid memory leaks
//     free(cfg);

//     // Now we can convert the magnitudes to bars
//     std::vector<int> bars(nbars, 0);
//     int binsPerBar = (int)magnitudes.size() / nbars;

//     for (int i = 0; i < nbars; ++i) {
//         float sum = 0;
//         for (int j = 0; j < binsPerBar; ++j) {
//             sum += magnitudes[i * binsPerBar + j];
//         }
        
//         float average = sum / binsPerBar;
        
//         // Scale the average to fit your terminal window height
//         // You might need a "sensitivity" multiplier here
//         int height = (int)(average * 10.0f); 
//         if (height > maxHeight) height = maxHeight;
        
//         bars[i] = height;
//     }
//     return bars;
// }

