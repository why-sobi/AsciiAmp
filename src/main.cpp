#define STB_IMAGE_IMPLEMENTATION

// standard includes
#include <iostream>
#include <vector>
#include <random>

// 3rd party includes
#include <termviz.hpp>
#include <taglib/fileref.h>
#include "stb_image.h"

// project includes
#include <temp.hpp>

int main() {
    const std::string dir = "C:/Users/shadows box/Downloads";
    std::vector<fs::path> mp3Files = getMP3Files(dir);

    termviz::clear_screen();
    termviz::Window thumbnail(1, 1, 180, 19);
    termviz::Window info(1, 20, 180, 5);

    for (const fs::path& mp3File : mp3Files) {
        TagLib::FileRef f(mp3File.c_str());
        if (!f.isNull() && f.tag()) {
            TagLib::Tag* tag = f.tag();
            info.print_msg("Title " + tag->title().to8Bit(true));
            info.print_msg("Artist " + tag->artist().to8Bit(true));
            info.print_msg("Album: " + tag->album().to8Bit(true));            
        }
        info.render();
    }

    termviz::reset_cursor();
}


/*
Put all headers in the include dir
"${CMAKE_CURRENT_SOURCE_DIR}libs/stb" 
*/

