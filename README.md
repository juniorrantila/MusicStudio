# MusicStudio

**MusicStudio** is a digital audio workstation.

![MusicStudio](Res/MusicStudio.png)

## Build instructions

### Prerequisites

#### macOS

    xcode-select install

#### Ubuntu

    sudo apt install freeglut3-dev libglew-dev libjack-jackd2-dev

### Setup:

    ./bootstrap
    source Toolchain/env

### Build:

    ninja -C build

### Run:

    ./build/<system-architecture>/music-studio
