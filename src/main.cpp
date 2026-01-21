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

using namespace std::chrono_literals;
namespace tv = termviz;
namespace Viz = termviz::Visualizer::Plots;

int main() {
    tv::clear_screen();
    std::vector<fs::path> musicLibrary = getMP3Files(MUSIC_PATH); // storing all the paths of the music (we don't create music objects yet to save memory)

    tv::Window fft(IMAGE_W + 1, 1, FULL_WINDOW_WIDTH - IMAGE_W - 1, IMAGE_H, "Visualizer");
    tv::Window title(1, IMAGE_H + 1, FULL_WINDOW_WIDTH - 1, TITLE_H, "Now Playing");
    tv::Window playback(1, IMAGE_H + TITLE_H + 1, FULL_WINDOW_WIDTH - 1, PLAYBACK_H, "Playback");

    int barWidth = 7;
    int maxBars = Viz::getMaxBars(fft, barWidth);
    int sample_window_size = 1024;
    // we'll consider the 70% width of the playback window
    // 15% space --------------70% playback------------ 15% space

    int playback_width = playback.get_w() * (0.7f);
    int starting_col   = playback.get_w() * (0.15f);
    int bar_width      = playback_width - 12;           // substracting indices of timestamps (current and end) + the playback border


    std::vector<termviz::COLOR> barColors(maxBars, termviz::COLOR(termviz::COLOR::BLUE)); // all bars blue

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
    int music_index = 0;
    bool prev;
    while (true) {
        Music music(musicLibrary[music_index]); // loading music
        playbackInfo = music;                   // creating music reference for playback (reduce casting cost)
        prev = false;

        screenInit(music, title);               // display everything at the start of the music

        // music related config
        config.sampleRate = music.sample_rate; // NOTE: When changing this we need to reconfigure our ma_device (since it'll be locked to previous settings)

        if (deviceInitialized) ma_device_uninit(&device);
        if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) continue; 
        deviceInitialized = true;

        // set the music's start time
        playbackInfo.startTime.store(std::chrono::steady_clock::now());

        // start music
        ma_device_start(&device); // creates its own thread for music playback

        std::thread playback_thread(runTimestamp, std::ref(playback), std::ref(music), std::ref(playbackInfo), playback_width, bar_width, starting_col);

        while (playbackInfo.isPlaying) { // this is falsed in our data_callback function 
            // next music
            char code = controller(playbackInfo, &device); 
            switch(code) { // rest of the controls are handled inside the function 
                case 'q': playback_thread.join(); return 0;
                case 'b': prev = true; playbackInfo.isPlaying = false; break;
                default: break;
            }
            
            if (!playbackInfo.pause.load()) {
                // equalizer stuff
                Viz::draw_bars(fft, getNbars(playbackInfo, cfg, maxBars, fft.get_h()), barWidth, barColors, '#');
                fft.render();
            }
        
            std::this_thread::sleep_for(25_FPS); 
        }   

        playback_thread.join();
        ma_device_stop(&device);
        music_index = prev ? music_index = (musicLibrary.size() + (music_index - 1)) % musicLibrary.size() : (music_index + 1) % musicLibrary.size();
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

