# AsciiAmp

A high-performance CLI music player featuring a real-time ASCII visualizer, built with C++, Miniaudio, and TagLib.

## 🚀 Quick Start

To get AsciiAmp running on your machine, follow these steps. This project uses Git Submodules to manage dependencies, so ensure you clone recursively.

### 1. Clone the Repository

```powershell
git clone --recursive https://github.com/why-sobi/AsciiAmp.git
cd AsciiAmp

```

### 2. Initialize Dependencies

If you forgot to clone with `--recursive`, or to ensure all sub-modules are ready, run:

```powershell
git submodule update --init --recursive

```

### 3. Build & Install TagLib (Local)

TagLib requires a local installation to its `dist` folder to provide the necessary headers and static libraries for the main build.

```powershell
# Configure TagLib
cmake -S external/taglib -B external/taglib/build -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="external/taglib/dist" -DCMAKE_BUILD_TYPE=Release -DWITH_ZLIB=OFF -DBUILD_SHARED_LIBS=OFF

# Compile
cmake --build external/taglib/build

# Install to external/taglib/dist
cmake --install external/taglib/build

```

### 4. Build AsciiAmp

Now you can build the main application:

```powershell
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .

```

---

## 🎹 Controls

| Key | Action |
| --- | --- |
| `Space` / `P` | Pause / Play |
| `Q` | Quit Safely |
| `B` | Previous Track |
| `Arrows` | Restart Track / Next Track |

---

## 🛠 Dependencies

AsciiAmp relies on the following incredible open-source libraries:

* **[miniaudio](https://github.com/mackron/miniaudio)** - Audio playback engine.
* **[TagLib](https://github.com/taglib/taglib)** - Audio metadata and bitrate parsing.
* **[kissfft](https://github.com/mborgerding/kissfft)** - Fast Fourier Transform for the visualizer.
* **[termviz](https://github.com/why-sobi/termviz)** - Terminal UI and ASCII rendering logic.
* **[minimp3](https://github.com/lieff/minimp3)** - Minimalistic MP3 decoder.
* **[stb](https://github.com/nothings/stb.git)** - Reading & Modifying Image
---

## Interface
<img width="1303" height="530" alt="image" src="https://github.com/user-attachments/assets/d656fde3-81ab-4bf5-bd3d-25b1abf1c650" />

---

## 📜 License

This project is licensed under the MIT License - see the LICENSE file for details.

---
