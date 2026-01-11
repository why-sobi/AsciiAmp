#pragma once

#include <vector>
#include <atomic>

#include <miniaudio.h>

#include <music.hpp>

struct Playback { // stores info for current music (a minimal reference to music object) it'll help reduce casting cost that the C libraries depend on (void* BS)
    std::vector<float>* samples; // no copy of actual samples
    std::atomic<size_t> playhead{0}; // read/write is atomic without mutex
    std::atomic<bool> pause{false};
    std::atomic<std::chrono::steady_clock::time_point> startTime;
    std::chrono::steady_clock::time_point pausedAt;
    bool isPlaying = false;

    void operator=(Music& obj) {
        this->samples = &(obj.monoSamples);
        this->playhead.store(0);
        this->isPlaying = true;
        this->pause.store(false);
    }
};

// we need to create a function named data_callback with the exact signature that the miniaudio will call
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    // 1. Cast the void pointer back to our C++ struct
    Playback* ctx = static_cast<Playback*>(pDevice->pUserData);
    
    // Safety check: if no samples or not playing or paused, output silence
    if (!ctx || !ctx->isPlaying || ctx->samples == nullptr || ctx->pause.load()) {
        memset(pOutput, 0, frameCount * sizeof(float));
        return; 
    }

    float* out = (float*)pOutput;
    size_t totalSamples = ctx->samples->size();

    // 2. Fill the buffer requested by the hardware (frameCount)
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        size_t currentPos = ctx->playhead.load(); // Get current playhead

        if (currentPos < totalSamples) {
            out[i] = (*ctx->samples)[currentPos];
            ctx->playhead.fetch_add(1); // Move playhead forward by 1
        } else {
            out[i] = 0.0f; // Song ended, fill with silence
            ctx->isPlaying = false;
        }
    }
}
