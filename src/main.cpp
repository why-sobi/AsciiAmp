// standard includes
#include <iostream>
#include <vector>
#include <random>

// 3rd party includes
#include <termviz.hpp>
#include <taglib/fileref.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// project includes
#include <temp.hpp>

int main() {
    const std::string dir = "C:/Users/shadows box/Downloads";
    std::vector<fs::path> mp3Files = getMP3Files(dir);

    std::cout << "Total MP3 files: " << mp3Files.size() << '\n';

    for (const fs::path& mp3File : mp3Files) {
        TagLib::FileRef f(mp3File.c_str());
        if (!f.isNull() && f.tag()) {
            TagLib::Tag* tag = f.tag();
            std::cout << "Title: " << tag->title().to8Bit(true) << std::endl;
            std::cout << "Artist: " << tag->artist().to8Bit(true) << std::endl;
            std::cout << "Album: " << tag->album().to8Bit(true) << std::endl;
            std::cout << "--------------------------" << std::endl;
        }
    }
}


/*
Put all headers in the include dir
"${CMAKE_CURRENT_SOURCE_DIR}libs/stb" 
*/

