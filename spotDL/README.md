## Music Ingestion Setup (spotDL)

This directory handles the automated downloading of music and metadata. The C++ TUI expects a specific folder structure to correctly parse album art and audio streams.

### 1. Environment Setup

Create a local Python virtual environment to keep the dependencies isolated from your system.

```bash
# Create the virtual environment
python -m venv venv

# Activate it (Windows)
.\venv\Scripts\activate

# Activate it (Linux/macOS)
source venv/bin/activate

```

### 2. Install Dependencies

Install the spotDL tool and the FFmpeg binaries required for audio processing.

```bash
# Install spotDL via pip
pip install spotdl

# Download local FFmpeg binaries (keeps your system PATH clean)
spotdl --download-ffmpeg

```

### 3. Downloading Music

To ensure the C++ engine can find your files, always download into the `music/` subdirectory. After *opening music folder*

```bash
# Download a single track or playlist
spotdl download [Spotify-URL]

```

---

### 📂 Expected Folder Structure

For the C++, ensure your `spotDL` folder looks like this:

```text
spotDL/
├── venv/                # Python virtual environment (Ignored by Git)
├── README.md            # This file
└── music/               # <--- C++ reads MP3s from here
    ├── Artist - Song1.mp3
    └── Artist - Song2.mp3

```

### ⚠️ Note on Git

The `music/`, `venv/`, and `ffmpeg` binaries are excluded from version control via the root `.gitignore` to prevent copyright issues and repository bloat.
