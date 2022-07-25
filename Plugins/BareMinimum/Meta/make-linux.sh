#!/bin/sh
last_dir=$PWD
cd ~/src/MusicStudio
mkdir -p Build/Plugins &&
zig c++ \
    -o Build/Plugins/BareMinimum.so \
    -std=c++20 \
    -fno-rtti \
    -fno-exceptions \
    -Wno-c99-designator \
    -O2 \
    -I Libraries \
    -shared \
    Libraries/Vst/AudioPlugin.cpp \
    Libraries/Vst/Vst.cpp \
    Plugins/BareMinimum/main.cpp
cd $last_dir
