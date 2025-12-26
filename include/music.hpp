# pragma once

#include <filesystem>
#include <vector>
#include <string_view>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>


inline constexpr char MUSIC_PATH[] = "../spotDL/music/";

namespace fs = std::filesystem;

struct Music {
    std::string title;
    std::string artist;
    std::string album;
    std::string duration;
    std::vector<uint8_t> coverArt;

    Music() {}
    Music(std::string t, std::string ar, std::string al, std::string d, std::vector<uint8_t> art) 
    // !NOT const & it'll mostly be used to read a lot of data instead of hardcoded data mutiple moving is faster (look into the extractMusicInfo function)
        : title(std::move(t)), artist(std::move(ar)), 
          album(std::move(al)), duration(std::move(d)), 
          coverArt(std::move(art)) {}
    
    Music(Music&& other) noexcept :
        title(std::move(other.title)),
        artist(std::move(other.artist)),
        album(std::move(other.album)),
        duration(std::move(other.duration)),
        coverArt(std::move(other.coverArt)) {}

    Music& move(std::string title, std::string artist, std::string album, std::string duration, std::vector<uint8_t> coverArt) {
        this->title = std::move(title);
        this->artist = std::move(artist);
        this->album = std::move(album);
        this->duration = std::move(duration);
        this->coverArt = std::move(coverArt);

        return *this;
    }
    
    friend std::ostream& operator << (std::ostream& out, const Music& object) {
        out << "Title: " << object.title 
        << "\nArtist: " << object.artist 
        << "\nAlbum: " << object.album 
        << "\nDuration: " << object.duration;
        return out;
    } 
};

std::vector<fs::path> getMP3Files(const std::string& dir) {
    std::vector<fs::path> mp3Files;

    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        throw std::invalid_argument("Directory does not exist: " + dir);
    }

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mp3") {
            mp3Files.push_back(entry.path());
        }
    }

    return mp3Files;
}


Music extractMusicInfo(const fs::path& path) {
    // 1. Open the file specifically as an MPEG file for ID3v2 access
    TagLib::MPEG::File f(path.c_str());
    
    std::string artist = "Unknown", title = "Unknown", album = "Unknown", duration = "0:00";
    std::vector<uint8_t> coverArt;

    if (f.isValid()) {
        // Text Metadata
        if (f.tag()) {
            TagLib::Tag *tag = f.tag();
            title = tag->title().to8Bit(true);
            artist = tag->artist().to8Bit(true);
            album = tag->album().to8Bit(true);
        }

        // Duration logic
        if (f.audioProperties()) {
            int sec = f.audioProperties()->lengthInSeconds();
            duration = std::to_string(sec / 60) + ":" + (sec % 60 < 10 ? "0" : "") + std::to_string(sec % 60);
        }

        // 2. Extract Album Art (APIC Frame)
        if (f.ID3v2Tag()) {
            auto frameList = f.ID3v2Tag()->frameList("APIC");
            if (!frameList.isEmpty()) {
                auto* frame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
                TagLib::ByteVector pictureData = frame->picture();
                
                // Copy binary data into our vector
                coverArt.assign(pictureData.data(), pictureData.data() + pictureData.size());
            }
        }
    }

    return Music(std::move(title), std::move(artist), std::move(album), std::move(duration), std::move(coverArt));
}

std::vector<Music> getMusicFromPath(const std::string& dir) {
    std::vector<fs::path> paths = getMP3Files(dir);
    std::vector<Music> music;
    music.reserve(paths.size());

    for (const fs::path& path : paths) {
        music.emplace_back(extractMusicInfo(path));
    }

    return music;
}