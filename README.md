# MusicStudio

**MusicStudio** is a digital audio workstation.

![MusicStudio](Res/MusicStudio.png)

## Build instructions

### Prerequisites

#### macOS

```

brew install ccache meson ninja-build

```

#### Ubuntu

```

sudo apt install clang ccache meson ninja-build freeglut3-dev libglew-dev libjack-jackd2-dev

```

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
