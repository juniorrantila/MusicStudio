# MusicStudio

**MusicStudio** is a digital audio workstation.

![MusicStudio](Res/MusicStudio.png)

## Prerequisites

### Ubuntu

```

sudo apt install clang ccache meson ninja-build freeglut3-dev libglew-dev libjack-jackd2-dev

```

## Build instructions

### Setup:

```sh

meson build

```

### Build:

```sh

ninja -C build

```

### Run:

```sh

./build/src/music-studio

```
