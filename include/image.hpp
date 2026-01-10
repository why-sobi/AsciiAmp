# pragma once

#include <iostream>
#include <vector>
#include <stb_image.h>
#include <termviz.hpp> // for colors

enum SAMPLE : uint8_t { 
    BILINEAR_INTERPOLATION,                                                 // extract features from neighbours and weighing their features based on distance
    BOX                                                                     // averaging over the kernel area
};

class Image {
    std::vector<uint8_t> rgb_pixels; // data is in 1D array [R,G,B,R,G,B ....] format
    int width = 0, height = 0, channels = 0;

    float sigmoid(float x, float k, float midpoint) {
        // x should be 0.0 to 1.0. k is the "punch" (contrast).
        // Center the curve at 0.5 (mid-tones)
        return 1.0f / (1.0f + exp(-k * (x - midpoint)));
    }

    std::vector<float> sampleBinaryInterpolation(float u, float v) {
        int x1 = std::clamp((int)floor(u), 0, width - 1);
        int y1 = std::clamp((int)floor(v), 0, height - 1);
        int x2 = std::min(x1 + 1, width - 1);
        int y2 = std::min(y1 + 1, height - 1);

        // 2. Fractional distances for weighting
        float dx = u - x1;
        float dy = v - y1;

        // 3. Sample the 4 pixels
        uint8_t* p11 = at(x1, y1); // Top-Left
        uint8_t* p21 = at(x2, y1); // Top-Right
        uint8_t* p12 = at(x1, y2); // Bottom-Left
        uint8_t* p22 = at(x2, y2); // Bottom-Right

        std::vector<float> result(3); // RGB for 1 pixel
        for (int i = 0; i < 3; i++) {
            // Linear interpolation in X, then Y (as per the standard formula)
            float top = (1.0f - dx) * p11[i] + dx * p21[i];
            float bot = (1.0f - dx) * p12[i] + dx * p22[i];
            result[i] = (1.0f - dy) * top + dy * bot;
        }
        return std::move(result);
    }

    std::vector<uint8_t> sampleBOX(int x, int y, float kernel_w, float kernel_h) {
        int startX = (int)(x * kernel_w);
        int endX   = (int)((x + 1) * kernel_w);
        int startY = (int)(y * kernel_h);
        int endY   = (int)((y + 1) * kernel_h);

        long sumR = 0, sumG = 0, sumB = 0;
        int count = 0;

        for (int ky = startY; ky < endY && ky < height; ky++) {
            for (int kx = startX; kx < endX && kx < width; kx++) {
                uint8_t* pixel = at(kx, ky);
                sumR += pixel[0];
                sumG += pixel[1];
                sumB += pixel[2];
                count++;
            }
        }

        std::vector<uint8_t> sample(3);
        sample[0] = (uint8_t)(sumR / count);
        sample[1] = (uint8_t)(sumG / count);
        sample[2] = (uint8_t)(sumB / count);

        return std::move(sample);
    }

    Image(int w, int h, int c, std::vector<uint8_t> pixels): width(w), height(h), channels(c), rgb_pixels(std::move(pixels)) {}
public:
    
    // Expects the returned TagLib::ByteVector pictureData = frame->picture(); converted into vector<uint8_t>
    Image(const std::vector<uint8_t>& compressed_data) {
        if (compressed_data.empty()) return;

        // stb_image decodes the JPEG/PNG bytes into raw RGB bytes
        unsigned char* decoded = stbi_load_from_memory(
            compressed_data.data(), 
            static_cast<int>(compressed_data.size()), 
            &width, &height, &channels, 
            3
        );

        if (decoded) {
            // Assign the raw C-pointer data to our C++ vector
            rgb_pixels.assign(decoded, decoded + (width * height * 3));
            channels = 3;
            stbi_image_free(decoded); // Clean up the raw C-pointer
        }
    }

    Image(const std::string& path) {
        // stb_image decodes the JPEG/PNG bytes into raw RGB bytes
        unsigned char* decoded = stbi_load(
            path.c_str(), 
            &width, &height, &channels, 
            3
        );

        if (decoded) {
            // Assign the raw C-pointer data to our C++ vector
            rgb_pixels.assign(decoded, decoded + (width * height * 3));
            channels = 3;
            stbi_image_free(decoded); // Clean up the raw C-pointer
        }
    }

    // Accessor for your processing functions
    uint8_t* at(int x, int y) {
        if (x < 0 || x >= width || y < 0 || y >= height) throw std::out_of_range("Index out of range access\n");
        return &rgb_pixels[(y * width + x) * 3];
    }

    float getLuminance(int x, int y) const { // so we can later assign each an ascii image according to its brightness
        if (x < 0 || x >= width || y < 0 || y >= height) return 0.0f;
        
        int index = (y * width + x) * 3;
        uint8_t r = rgb_pixels[index];
        uint8_t g = rgb_pixels[index + 1];
        uint8_t b = rgb_pixels[index + 2];

        // Standard luminosity formula (Weights: G > R > B)
        return (0.2126f * r + 0.7152f * g + 0.0722f * b) / 255.0f;
    }

    void downScale(int new_width, int new_height, SAMPLE method = SAMPLE::BOX) { 
        if (new_width <= 0 || new_width > width || new_height <= 0 || new_height > height) {
            throw std::invalid_argument("New dimensions are not valid for down scaling\n");
        }

        std::vector<uint8_t> new_pixels;
        new_pixels.reserve(new_width * new_height * 3);

        // How many 'old' pixels fit into one 'new' pixel
        float kernel_width = (float)width / new_width;
        float kernel_height = (float)height / new_height;

        // Loop through the NEW grid (one pixel in new = kernel size in old)
        for (int y = 0; y < new_height; y++) {
            for (int x = 0; x < new_width; x++) {
                // kernel image indexes on old

                if (method == SAMPLE::BILINEAR_INTERPOLATION) {
                    float u = x * kernel_width;
                    float v = y * kernel_height;

                    std::vector<float> sampled = sampleBinaryInterpolation(u, v);
                    
                    // Convert back to uint8_t and push into the new buffer
                    new_pixels.push_back((uint8_t)std::clamp(sampled[0], 0.0f, 255.0f));
                    new_pixels.push_back((uint8_t)std::clamp(sampled[1], 0.0f, 255.0f));
                    new_pixels.push_back((uint8_t)std::clamp(sampled[2], 0.0f, 255.0f));
                } else if (method == SAMPLE::BOX) {
                    std::vector<uint8_t> sampled = sampleBOX(x, y, kernel_width, kernel_height);
                    new_pixels.insert(new_pixels.end(), sampled.begin(), sampled.end());
                }
            }
        }

        this->rgb_pixels = std::move(new_pixels);
        this->width = new_width;
        this->height = new_height;
    }

    std::pair<std::vector<char>, std::vector<termviz::COLOR>> toAscii(float contrast=8.5f, float brightness=10.0f, float midpoint=0.35f) {
        // Increase contrast to make it punchier
        // Decrease brightness to "darken" the detection
        
        static constexpr char CHAR_MAP[] = "o%&8#@$";
        
        // We use 'brightness' to shift the midpoint (default is 0.5f)
        midpoint = midpoint - (brightness / 100.0f); // tweak brightness to shift the curve (lower midpoint makes everything move towards a light color range)

        std::vector<char> chars;
        std::vector<termviz::COLOR> colors;

        chars.reserve(this->width * this->height);
        colors.reserve(this->width * this->height); 

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) { // moving pixel by pixel
                uint8_t* pixel = at(x, y);

                // 1. Normalize to 0.0 - 1.0 range
                float r_norm = pixel[0] / 255.0f;
                float g_norm = pixel[1] / 255.0f;
                float b_norm = pixel[2] / 255.0f;

                // 2. Apply Sigmoid (using 'contrast' as the k-factor)
                r_norm = sigmoid(r_norm, contrast, midpoint);
                g_norm = sigmoid(g_norm, contrast, midpoint);
                b_norm = sigmoid(b_norm, contrast, midpoint);

                // 3. Scale back and Clamp with your Floor
                int r = std::clamp((int)(r_norm * 255.0f), 15, 255);
                int g = std::clamp((int)(g_norm * 255.0f), 15, 255);
                int b = std::clamp((int)(b_norm * 255.0f), 15, 255);
                
                // general formula idk how it was derived
                float luminance = (0.2126f * r + 0.7152f * g + 0.0722f * b) / 255.0f;

                // based on luminance we'll map the pixel with some char from CHAR_MAP 0 being the dimmest to eventually getting lighter
                int index = static_cast<int>(luminance * (sizeof(CHAR_MAP) - 2)); // as each char takes 1 byte we do not need to divide by CHAR_MAP[0]
                
                // Add ANSI TrueColor codes
                // Format: \033[38;2;R;G;Bm
                colors.emplace_back(r, g, b); // creates COLOR object and stores (reduces copies since everything inplace)
                chars.emplace_back(CHAR_MAP[index]);
            }
        }
        return {std::move(chars), std::move(colors)};
    }


};