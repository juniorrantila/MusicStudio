soundio_backend_deps = []
soundio_c_args = [
  '-Wno-unused-parameter',
  '-Wno-sign-compare',
]
soundio_platform_sources = []

if target_machine.system() == 'windows'
  wasapi_dep = dependency('wasapi', required: true, static: true)
  soundio_backend_deps += wasapi_dep
  soundio_c_args += '-DSOUNDIO_HAVE_WASAPI=1'
  soundio_platform_sources += './wasapi.c'
elif target_machine.system() == 'darwin'
  soundio_backend_deps += dependency('corefoundation', required: true, static: true)
  soundio_backend_deps += dependency('audiounit', required: true, static: true)

  coreaudio_dep = dependency('coreaudio', required: true, static: true)
  soundio_backend_deps += coreaudio_dep
  soundio_c_args += '-DSOUNDIO_HAVE_COREAUDIO=1'
  soundio_platform_sources += './coreaudio.c'
else
  jack_dep = dependency('jack', required: false, static: true)
  pulseaudio_dep = dependency('pulseaudio', required: false, static: true)
  alsa_dep = dependency('alsa', required: false, static: true)
  if jack_dep.found()
    soundio_backend_deps += jack_dep
    soundio_c_args += '-DSOUNDIO_HAVE_JACK=1'
    soundio_platform_sources += './jack.c'
  endif
  if pulseaudio_dep.found()
    soundio_backend_deps += pulseaudio_dep
    soundio_c_args += '-DSOUNDIO_HAVE_PULSEAUDIO=1'
    soundio_platform_sources += './pulseaudio.c'
  endif
  if alsa_dep.found()
    soundio_backend_deps += alsa_dep
    soundio_c_args += '-DSOUNDIO_HAVE_ALSA=1'
    soundio_platform_sources += './alsa.c'
  endif
  if not jack_dep.found() and not pulseaudio_dep.found() and not alsa_dep.found()
    warning('could not find any backend libraries (jack, pulseaudio or alsa)')
  endif
endif

soundio_lib = static_library('soundio', [
    soundio_platform_sources,
    'channel_layout.c',
    'dummy.c',
    'os.c',
    'ring_buffer.c',
    'SoundIo.c',
    'util.c',
  ], c_args: [
    soundio_c_args
  ], dependencies: [
    soundio_backend_deps
  ])

soundio_dep = declare_dependency(
  link_with: soundio_lib,
  include_directories: '..'
)
