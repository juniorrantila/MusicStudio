
cpp = meson.get_compiler('cpp')
libJack = cpp.find_library('jack', required: true)
libSDL2 = cpp.find_library('SDL2', required: true)
libX11 = cpp.find_library('X11', required: true)

music_studio = executable('MusicStudio', [
    'Host.cpp',
    'Plugin.cpp',
    'main.cpp',
    'Imgui.cpp',
    'MainWindow.cpp'
  ],
  include_directories: includes,
  link_with: [libVstHost, libGUI, libImgui],
  dependencies: [libJack, libSDL2, libX11],
  )
