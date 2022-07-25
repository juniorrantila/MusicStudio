#!/bin/sh
last_dir=$PWD
cd ~/src/MusicStudio
mkdir -p Build/Plugins &&
zig c++ \
    -o Build/Plugins/AudioPlugin.so \
    -I Dependencies \
    -I /usr/include/SDL2 \
    -I Dependencies/imgui \
    -I Libraries \
    -I . \
    -std=c++20 \
    -fno-rtti \
    -fno-exceptions \
    -Wno-c99-designator \
    -O2 \
    -shared \
    Plugins/AudioPlugin/AudioPlugin.cpp \
    Plugins/AudioPlugin/main.cpp \
    Fonts/OxaniumLight.cpp \
    Libraries/Vst/AudioPlugin.cpp \
    Libraries/Vst/Vst.cpp \
    Dependencies/imgui-knobs/imgui-knobs.cpp \
    Dependencies/imgui/backends/imgui_impl_sdl.cpp \
    Dependencies/imgui/backends/imgui_impl_sdlrenderer.cpp \
    Dependencies/imgui/imgui.cpp \
    Dependencies/imgui/imgui_demo.cpp \
    Dependencies/imgui/imgui_draw.cpp \
    Dependencies/imgui/imgui_tables.cpp \
    Dependencies/imgui/imgui_widgets.cpp \
    -lSDL2 \
    &&
cp Build/Plugins/AudioPlugin.so ~/.vst
cd $last_dir
