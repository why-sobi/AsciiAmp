#define MINIMP3_IMPLEMENTATION 
#define MINIAUDIO_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

// standard includes
#include <iostream>
#include <vector>

// 3rd party includes
#include <termviz.hpp>

// project includes
#include <music.hpp>
#include <image.hpp>
#include <playback.hpp>
#include <utils.hpp>

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <conio.h>

using namespace std::chrono_literals;
namespace tv = termviz;
namespace Viz = termviz::Visualizer::Plots;

void updatePlaybackUI(tv::Window& win, const std::vector<int>& heights) {
    win.clean_buffer();
    for (size_t i = 0; i < heights.size(); ++i) {
        win.print(0, i * 4, std::to_string(heights[i]));
    }
    win.render(true);
}

int main() {
    tv::clear_screen();
    std::vector<fs::path> musicLibrary = getMP3Files(MUSIC_PATH);

    tv::Window fft(IMAGE_W + 1, 1, FULL_WINDOW_WIDTH - IMAGE_W - 1, IMAGE_H, "Visualizer");
    tv::Window title(1, IMAGE_H + 1, FULL_WINDOW_WIDTH - 1, TITLE_H, "Now Playing");
    tv::Window playback(1, IMAGE_H + TITLE_H + 1, FULL_WINDOW_WIDTH - 1, 10, "Playback");

    int barWidth = 7;
    int maxBars = Viz::getMaxBars(fft, barWidth);
    int sample_window_size = 1024;

    std::vector<termviz::COLOR> barColors(maxBars, termviz::COLOR(termviz::COLOR::BLUE));

    // objects needed
    Playback playbackInfo;
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;   // since std::vector<float> for samples
    config.playback.channels = 1;               // Mono (since every music was converted into mono)
    config.dataCallback      = data_callback;   // The function we wrote in playback.hpp
    config.pUserData         = &playbackInfo;   // the info it'll send to the data_callback function
    
    kiss_fft_cfg cfg = kiss_fft_alloc(sample_window_size, 0, nullptr, nullptr); 

    bool deviceInitialized = false;
    ma_device device; // speaker

    // in loop we update rest of the config as necessary

    // ------------------------ PLAYBACK LOOP ----------------------------
    for (const fs::path path : musicLibrary) {
        Music music(path);                      // loading music
        playbackInfo = music;                   // creating music reference for playback (reduce casting cost)

        // print and setup everything else
        printCover(music);
        title.clean_buffer();
        title.print(0, getPadding(music.title, title.get_w()), format(toUpper(music.title), BOLD));
        title.print(1, getPadding(music.artist, title.get_w()), format(toUpper(music.artist), UNDERLINE));
        title.print(2, getPadding(music.album, title.get_w()), format(toUpper(music.album)));
        title.render(true); 

        // music related config
        config.sampleRate = music.sample_rate; // NOTE: When changing this we need to reconfigure our ma_device (since it'll be locked to previous settings)

        if (deviceInitialized) ma_device_uninit(&device);
        if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) continue; 
        deviceInitialized = true;

        // start music
        ma_device_start(&device); // creates its own thread for music playback

        while (playbackInfo.isPlaying) { // this is falsed in our data_callback function 
            // next music
            if (kbhit()) { // if keyboard hit
                int code = _getch();
                if (code == 0 || code == 224) { // special keys
                    int arrow = _getch(); // Catch the second value
                    switch (arrow) {
                        case 72: // UP
                            // Maybe increase volume?
                            break;
                        case 80: // DOWN
                            // Maybe decrease volume?
                            break;
                        case 75: // LEFT
                            // Seek backward?
                            break;
                        case 77: // RIGHT
                            playbackInfo.isPlaying = false; // Skip to next song
                            break;
                    }
                } else { // Handle regular keys
                    if (code == 'q') return 0;
                    if (code == ' ') /* pause logic */;
                }
            }

            // equalizer stuff
            Viz::draw_bars(fft, getNbars(playbackInfo, cfg, maxBars, fft.get_h()), barWidth, barColors);
            fft.render();
        
            std::this_thread::sleep_for(30_FPS); 
        }   

        ma_device_stop(&device);

    }

    free(cfg);

    tv::reset_cursor();
    return 0;
}


/*
Put all headers in the include dir
"${CMAKE_CURRENT_SOURCE_DIR}libs/stb" 
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/kissfft" 

*/
