libVst = library('Vst', [
    'AudioPlugin.cpp',
    'Vst.cpp',
  ],
  include_directories: includes,
  link_with: [libJR])

libVstHost = library('VstHost', [
    'VstHost.cpp'
  ],
  include_directories: includes,
  link_with: [libVst])
