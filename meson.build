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

cpp = meson.get_compiler('cpp')

add_project_arguments(cpp.get_supported_arguments([
  '-Wno-c99-designator',
  '-Wno-c99-extensions',
  '-Wno-format-pedantic',
  '-Wno-gnu-anonymous-struct',
  '-Wno-gnu-case-range',
  '-Wno-gnu-conditional-omitted-operand',
  '-Wno-gnu-statement-expression',
  '-Wno-gnu-zero-variadic-macro-arguments',
  '-Wno-keyword-macro',
  '-Wno-user-defined-literals',
  '-Wno-nested-anon-types',
  '-Werror=return-type',
  '-Werror=switch',
]), language: ['objcpp', 'cpp', 'c'])

if target_machine.system() == 'darwin'
  add_languages('objcpp')
  add_project_arguments([
    '-DGL_SILENCE_DEPRECATION=1',
    '-ffreestanding',
    '-nostdinc++',
  ], language: ['objcpp', 'cpp', 'c'])
endif

add_project_arguments(cpp.get_supported_arguments([
  '-fobjc-arc',
]), language: ['objcpp', 'objc'])

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

c_dep = [
  cpp.find_library('c', static: true),
  cpp.find_library('m', static: true),
]

opengl_dep = [dependency('opengl', static: true)]
if target_machine.system() != 'darwin'
  glew_proj = subproject('glew', default_options: [
    'warning_level=0',
    'default_library=static',
    'buildtype=release',
    'strip=true',
    'egl=disabled',
    'glu=disabled',
  ])
  opengl_dep += glew_proj.get_variable('glew_dep')
  if target_machine.system() != 'windows'
    opengl_dep += dependency('gl', static: true)
  endif
endif

bundle_me = []
bundle_libs = []

subdir('CoreLibraries')

subdir('BuildTools')
subdir('Fonts')

subdir('Libraries')

bundled_resources_lib = static_library('bundled-resources', [
    custom_target('bundle',
      output: 'Bundle.cpp',
      input: bundle_me,
      command: [
        make_bundle_exe,
        meson.project_build_root() / '@OUTPUT@',
        '@INPUT@'
      ]),
  ], dependencies: [
    ty_dep,
    bundle_dep,
  ], link_with: bundle_libs)

bundle_dep += declare_dependency(
  link_with: bundled_resources_lib
)

subdir('Tools')
subdir('Plugins')
subdir('src')
