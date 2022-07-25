#!/bin/sh
zig c++ \
    -o Plugins/BareMinimum/BareMinimum.dll \
    -std=c++20 \
    -fno-rtti \
    -fno-exceptions \
    -Wno-c99-designator \
    -O2 \
    -shared \
    --target=x86_64-windows-gnu \
    -I ../Libraries \
    -I ../Libraries/LibJR \
    ../Libraries/Vst/AudioPlugin.cpp \
    ../Libraries/Vst/Vst.cpp \
    ../Plugins/BareMinimum/main.cpp
