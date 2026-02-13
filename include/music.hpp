# pragma once

#include <filesystem>
#include <vector>
#include <string_view>
#include <atomic>

#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <minimp3.h>
#include <minimp3_ex.h>
#include <miniaudio.h>

namespace fs = std::filesystem;

struct Music {
    std::vector<uint8_t> coverArt;
    std::vector<float> monoSamples;
    std::string title;
    std::string artist;
    std::string album;
    std::string duration;
    std::string bitrate;
    int channels;
    int sample_rate;

    Music() {}
    
    Music(const fs::path& path) {
        { // have to make the MPEG go out of scope so it leaves the file and minimp3 can use it
            TagLib::MPEG::File f(path.c_str());
        
            artist = "Unknown", title = "Unknown", album = "Unknown", duration = "0:00";

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
                    bitrate = std::to_string(f.audioProperties()->bitrate());                                               // kbps
                    channels = f.audioProperties()->channels();                                                             // 1 = Mono, 2 = Stereo
                    sample_rate = f.audioProperties()->sampleRate();
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
        }
        load(path.string());
    }
    
    Music(int sample_rate, std::string t, std::string ar, std::string al, std::string d, std::vector<uint8_t> art, std::vector<float> samples) 
    // !NOT const & it'll mostly be used to read a lot of data instead of hardcoded data mutiple moving is faster (look into the extractMusicInfo function)
    // samples are expected to be mono
    : sample_rate(sample_rate),
        title(std::move(t)), artist(std::move(ar)), 
        album(std::move(al)), duration(std::move(d)), 
        coverArt(std::move(art)), monoSamples(std::move(samples)) {}
    
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
        out << "Sample Rate: " << object.sample_rate
        << "\nTitle: " << object.title 
        << "\nArtist: " << object.artist 
        << "\nAlbum: " << object.album 
        << "\nDuration: " << object.duration;
        return out;
    } 

private:   
    void load(const std::string& path) {
        mp3dec_t mp3d;
        mp3dec_init(&mp3d); 
        mp3dec_file_info_t info;

        int error = mp3dec_load(&mp3d, path.c_str(), &info, NULL, NULL);

        if (error != 0) {
            throw std::runtime_error("minimp3 error code: " + std::to_string(error));
        }

        this->monoSamples.reserve(info.samples / channels);

        if (channels == 2) { // [-1, 1] range for fft
            for (size_t i = 0; i < info.samples; i += 2) {
                float mono = (info.buffer[i] + info.buffer[i + 1]) / 65536.0f;
                this->monoSamples.push_back(mono);
            }
        } else {
            for (size_t i = 0; i < info.samples; i++) {
                this->monoSamples.push_back(info.buffer[i] / 32768.0f);
            }
        }

        free(info.buffer); 
    }
};

std::vector<fs::path> getMP3Files(const std::string& dir = "../music") {
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