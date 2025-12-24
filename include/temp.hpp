# pragma once

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

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


