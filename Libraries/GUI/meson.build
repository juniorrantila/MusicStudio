
cpp = meson.get_compiler('cpp')
libX11 = cpp.find_library('X11', required: true)
# libSDL = cpp.find_library('SDL2', required: true)

libGUI = library('GUI', [
    'Window.cpp'
  ],
  include_directories: includes,
  link_with: [libJR],
  dependencies: [libX11]
  )
