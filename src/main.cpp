#define STB_IMAGE_IMPLEMENTATION

// standard includes
#include <iostream>
#include <vector>
#include <string_view>
#include <random>

// 3rd party includes
#include <termviz.hpp>

// project includes
#include <music.hpp>
#include <image.hpp>

template <typename T>
std::ostream& operator << (std::ostream& out, const std::vector<T>& obj) {
    for (const T& val : obj) { out << val; }
    return out;
} 

void printCover(const Music& music) { // wrapping in function release memory asap
    int window_width = 70, window_height = 25; // as height is double than width in terminal
    termviz::Window window(1, 1, window_width, window_height);

    Image img(music.coverArt);
    img.downScale(window.get_w(), window.get_h());

    auto [characters, colors] = img.toAscii();
    termviz::Visualizer::Plots::draw_frame(window, characters, colors);
    window.render();
} 

int main() {
    termviz::clear_screen();
    std::vector<Music> musicLibrary = getMusicFromPath(MUSIC_PATH);

    for (const Music& music : musicLibrary) {
        printCover(music);
            
        termviz::reset_cursor();
        std::cout << music.title << '\n'; 

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}


/*
Put all headers in the include dir
"${CMAKE_CURRENT_SOURCE_DIR}libs/stb" 
*/

