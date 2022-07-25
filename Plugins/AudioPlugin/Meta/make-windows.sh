#!/bin/sh
zig c++ \
    -o Plugins/AudioPlugin/AudioPlugin.dll \
    -I ../Dependencies \
    -I ../Dependencies/imgui \
    -I ../Dependencies/SDL2Windows/SDL2 \
    -I ../Libraries \
    -I ../Libraries/LibJR \
    -I .. \
    -I . \
    -std=c++20 \
    -fno-rtti \
    -fno-exceptions \
    -Wno-c99-designator \
    -O2 \
    -shared \
    --target=x86_64-windows-gnu \
    -lcfgmgr32 \
    -lgdi32 \
    -limm32 \
    -lkernel32 \
    -lole32 \
    -loleaut32 \
    -lsetupapi \
    -lshell32 \
    -lversion \
    -lwinmm \
    ../Plugins/AudioPlugin/AudioPlugin.cpp \
    ../Plugins/AudioPlugin/main.cpp \
    Fonts/OxaniumLight.cpp \
    ../Libraries/Vst/AudioPlugin.cpp \
    ../Libraries/Vst/Vst.cpp \
    ../Dependencies/imgui-knobs/imgui-knobs.cpp \
    ../Dependencies/imgui/backends/imgui_impl_sdl.cpp \
    ../Dependencies/imgui/backends/imgui_impl_sdlrenderer.cpp \
    ../Dependencies/imgui/imgui.cpp \
    ../Dependencies/imgui/imgui_draw.cpp \
    ../Dependencies/imgui/imgui_tables.cpp \
    ../Dependencies/imgui/imgui_widgets.cpp \
    ../Dependencies/SDL2Windows/SDL2/lib/libSDL2.a \
