#include <termviz.hpp>
#include <image.hpp>
#include <music.hpp>

inline extern constexpr int FULL_WINDOW_WIDTH = 229;
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
    int padding = ((window_w - str.length()) / 2) - 1;
    if (padding < 0) padding = 0;

    return padding;
}