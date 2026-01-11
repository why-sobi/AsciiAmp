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


void runTimestamp(tv::Window& playback, const Music& music, const Playback& playbackInfo, int playback_width, int bar_width, int starting_col) {
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
    int music_index = 0;
    bool prev;
    while (true) {
        Music music(musicLibrary[music_index]); // loading music
        playbackInfo = music;                   // creating music reference for playback (reduce casting cost)
        prev = false;

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

        // set the music's start time
        playbackInfo.startTime.store(std::chrono::steady_clock::now());

        // start music
        ma_device_start(&device); // creates its own thread for music playback

        std::thread playback_thread(runTimestamp, std::ref(playback), std::ref(music), std::ref(playbackInfo), playback_width, bar_width, starting_col);

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
                            playbackInfo.playhead.store(0);
                            playbackInfo.startTime.store(std::chrono::steady_clock::now());
                            break;
                        case 77: // RIGHT
                            playbackInfo.isPlaying = false; // Skip to next song
                            break;
                    }
                } else { // Handle regular keys
                    if (code == 'q' || code == 'Q') {
                        playbackInfo.isPlaying = false; 
                        playback_thread.join(); 
                        ma_device_stop(&device); 
                        tv::reset_cursor(); 
                        return 0;
                    } 
                    else if (code == 'b' || code == 'B') {prev = true; playbackInfo.isPlaying = false; break; }  
                    else if (code == 'p' || code == 'P' || code == ' ') { 
                        bool paused = playbackInfo.pause.load();
                        if (!paused) {
                            playbackInfo.pausedAt = std::chrono::steady_clock::now();
                        } else {
                            auto now = std::chrono::steady_clock::now();
                            // pushing start time
                            auto pause_duration = now - playbackInfo.pausedAt;
                            // Shift startTime forward by the duration of the pause
                            playbackInfo.startTime.store(playbackInfo.startTime.load() + pause_duration);
                        }

                        playbackInfo.pause.store(!paused);
                    }
                }
            }
            
            
            if (!playbackInfo.pause.load()) {
                // equalizer stuff
                Viz::draw_bars(fft, getNbars(playbackInfo, cfg, maxBars, fft.get_h()), barWidth, barColors);
                fft.render();
            }
        
            std::this_thread::sleep_for(30_FPS); 
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
