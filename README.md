# MusicStudio

**MusicStudio** is a digital audio workstation.

![MusicStudio](Res/MusicStudio.png)

## Build instructions

### Prerequisites

#### macOS

    brew install ccache meson ninja-build

#### Ubuntu

    sudo apt install ccache meson ninja-build clang freeglut3-dev libglew-dev libjack-jackd2-dev

### Setup:

    meson setup build

### Build:

    ninja -C build

### Run:

    ./build/src/music-studio
