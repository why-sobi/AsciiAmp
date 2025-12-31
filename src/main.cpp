// standard includes
#include <iostream>
#include <vector>

// 3rd party includes
#include <termviz.hpp>

// project includes
#include <music.hpp>
#include <image.hpp>
#include <utils.hpp>

int main() {
    termviz::clear_screen();
    std::vector<Music> musicLibrary = getMusicFromPath(MUSIC_PATH);
    // printCover(musicLibrary[musicLibrary.size() - 1]);

    termviz::Window fft(IMAGE_W + 1, 1, FULL_WINDOW_WIDTH - IMAGE_W - 1, IMAGE_H, "Visualizer");
    termviz::Window title(1, IMAGE_H + 1, FULL_WINDOW_WIDTH - 1, TITLE_H);
    termviz::Window playback(1, IMAGE_H + TITLE_H + 1, FULL_WINDOW_WIDTH - 1, 10, "Playback");


    for (const Music& music : musicLibrary) {
        printCover(music);
        
        title.clean_buffer();
        title.print(0, getPadding(music.title, title.get_w()), format(toUpper(music.title), BOLD));
        title.print(1, getPadding(music.artist, title.get_w()), format(toUpper(music.artist), UNDERLINE));
        title.print(2, getPadding(music.album, title.get_w()), format(toUpper(music.album)));

        title.render(true); 

        for (int i = 0; i < 10; i++) {
            draw_random_bars(fft);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    termviz::reset_cursor();
    std::cout << "Done!";

    return 0;
}


/*
Put all headers in the include dir
"${CMAKE_CURRENT_SOURCE_DIR}libs/stb" 
*/

