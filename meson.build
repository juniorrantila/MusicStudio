project('MusicStudio', ['cpp', 'c'],
  default_options: [
    'cpp_std=c++20',
    'c_std=c11',
    'cpp_eh=none',
    'cpp_rtti=false',
    'default_library=static',
    'prefer_static=true',
    'warning_level=3',
    'buildtype=release',
    'strip=true',
  ])

add_global_arguments([
  '-Wno-c99-designator',
  '-Wno-c99-extensions',
  '-Wno-format-pedantic',
  '-Wno-gnu-anonymous-struct',
  '-Wno-gnu-case-range',
  '-Wno-gnu-conditional-omitted-operand',
  '-Wno-gnu-designator',
  '-Wno-gnu-statement-expression-from-macro-expansion',
  '-Wno-gnu-zero-variadic-macro-arguments',
  '-Wno-keyword-macro',
  '-Wno-user-defined-literals',
], language: ['cpp', 'c'])

freetype2_proj = subproject('freetype2', default_options: [
  'warning_level=0',
  'default_library=static',
  'buildtype=release',
  'strip=true',
  'brotli=disabled',
  'bzip2=disabled',
  'harfbuzz=disabled',
  'mmap=enabled',
  'png=disabled',
  'tests=disabled',
  'zlib=disabled',
])
freetype2_dep = freetype2_proj.get_variable('freetype_dep')

if target_machine.system() == 'darwin'
  glew_dep = [
    dependency('glew', static: true),
    dependency('opengl', static: true),
  ]
else
  glew_proj = subproject('glew', default_options: [
    'warning_level=0',
    'default_library=static',
    'buildtype=release',
    'strip=true',
    'egl=disabled',
    'glu=disabled',
  ])
  glew_dep = glew_proj.get_variable('glew_dep')
endif

sdl2_dep = dependency('sdl2', required: target_machine.system() == 'darwin', static: true)
if not sdl2_dep.found()
  sdl2_proj = subproject('sdl2', default_options: [
    'warning_level=0',
    'default_library=static',
    'buildtype=release',
    'strip=true',
    'test=false',
    'run_test=false',
    'assertions=disabled',
    'use_threads=disabled',
    'use_atomic=disabled',
    'use_audio=disabled',
    'use_audio_alsa=disabled',
    'use_audio_pulseaudio=disabled',
    'use_audio_jack=disabled',
    'use_audio_pipewire=disabled',
    'use_cpuinfo=disabled',
    'use_events=enabled',
    'use_file=disabled',
    'use_joystick=disabled',
    'use_joystick_dinput=disabled',
    'use_joystick_xinput=disabled',
    'use_joystick_wgi=disabled',
    'use_joystick_virtual=disabled',
    'use_haptic=disabled',
    'use_hidapi=disabled',
    'use_sensor=disabled',
    'use_loadso=disabled',
    'use_locale=disabled',
    'use_render=enabled',
    'use_timers=enabled',
    'use_video=enabled',
    'use_video_x11=enabled',
    'use_video_wayland=disabled',
    'use_video_wayland_libdecor=disabled',
    'use_video_opengl=enabled',
    'use_video_openglesv2=disabled',
    'use_video_vulkan=disabled',
    'use_video_offscreen=disabled',
    'use_power=disabled',
    'use_filesystem=disabled',
    'use_dummies=false',
    'prefer_dlopen=false',
  ])
  sdl2_dep = sdl2_proj.get_variable('sdl2_dep')
endif

subdir('Libraries')

subdir('Tools')
subdir('Fonts')

subdir('Plugins')

subdir('src')
