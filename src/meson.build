
cpp = meson.get_compiler('cpp')
libJack = cpp.find_library('jack', required: true)

music_studio = executable('MusicStudio', [
    'Host.cpp',
    'Plugin.cpp',
    'main.cpp',
  ],
  include_directories: includes,
  link_with: [libVstHost, libGUI],
  dependencies: [libJack],
  )
