#define MINIMP3_IMPLEMENTATION 
#define STB_IMAGE_IMPLEMENTATION

// standard includes
#include <iostream>
#include <vector>

// 3rd party includes
#include <termviz.hpp>

// project includes
#include <music.hpp>
#include <image.hpp>
#include <utils.hpp>

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

// Using namespace aliases to make the code more readable
using namespace std::chrono_literals;
namespace tv = termviz;

void updatePlaybackUI(tv::Window& win, const std::vector<int>& heights) {
    win.clean_buffer();
    for (size_t i = 0; i < heights.size(); ++i) {
        win.print(0, i * 4, std::to_string(heights[i]));
    }
    win.render(true);
}

int main() {
    tv::clear_screen();
    auto musicLibrary = getMP3Files(MUSIC_PATH);
    kiss_fft_cfg cfg = kiss_fft_alloc(1024, 0, nullptr, nullptr);

    // 1. UI Layout Setup
    tv::Window fft(IMAGE_W + 1, 1, FULL_WINDOW_WIDTH - IMAGE_W - 1, IMAGE_H, "Visualizer");
    tv::Window title(1, IMAGE_H + 1, FULL_WINDOW_WIDTH - 1, TITLE_H, "Now Playing");
    tv::Window playback(1, IMAGE_H + TITLE_H + 1, FULL_WINDOW_WIDTH - 1, 10, "Playback");

    int barWidth = 7;
    int maxBars = tv::Visualizer::Plots::getMaxBars(fft, barWidth);

    for (const auto& path : musicLibrary) {
        Music music(path);
        
        // 2. Track Header Display
        printCover(music);
        title.clean_buffer();
        title.print(0, getPadding(music.title, title.get_w()), format(toUpper(music.title), BOLD));
        title.print(1, getPadding(music.artist, title.get_w()), format(toUpper(music.artist), UNDERLINE));
        title.print(2, getPadding(music.album, title.get_w()), format(toUpper(music.album)));
        title.render(true); 

        // 3. Playback Loop
        // size_t playhead = 0;
        // const int samplesPerFrame = music.sample_rate / 60; 
        
        // while (playhead + 1024 < music.monoSamples.size()) {
        //     auto frameStart = std::chrono::steady_clock::now();

        //     // A. Analysis
        //     std::vector<int> barHeights = getVizBars(
        //         playhead, 
        //         music.monoSamples, 
        //         cfg, 
        //         maxBars, 
        //         fft.get_h()
        //     );

        //     // B. Rendering
        //     tv::Visualizer::Plots::draw_bars(fft, barHeights, barWidth);
        //     fft.render(true);
        //     updatePlaybackUI(playback, barHeights);

        //     // C. Playback Progression
        //     playhead += samplesPerFrame;

        //     // D. Smart Timing (Compensate for processing time)
        //     auto frameEnd = std::chrono::steady_clock::now();
        //     std::this_thread::sleep_for(16ms - (frameEnd - frameStart));

        //     // E. Basic Input (Simulated - replace with your actual input check)
        //     // if (tv::poll_key() == 'n') break; 
        // }

        std::this_thread::sleep_for(seconds(5));
    }

    kiss_fft_free(cfg);
    tv::reset_cursor();
    return 0;
}


/*
Put all headers in the include dir
"${CMAKE_CURRENT_SOURCE_DIR}libs/stb" 
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/kissfft" 

*/

