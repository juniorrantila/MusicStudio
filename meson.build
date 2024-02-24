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
  '-Wno-gnu-statement-expression-from-macro-expansion',
  '-Wno-gnu-zero-variadic-macro-arguments',
  '-Wno-keyword-macro',
  '-Wno-user-defined-literals',
  '-Wno-nested-anon-types',
  '-Werror=return-type',
  '-ffreestanding',
  '-nostdinc++',
  '-Werror=switch',
]), language: ['objcpp', 'cpp', 'c'])

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

c_dep = cpp.find_library('c', static: true)

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
endif

embed_gen = generator(
  find_program('xxd'),
  output: '@PLAINNAME@.h',
  arguments: ['-i', '-n', '@PLAINNAME@', '@INPUT@', '@OUTPUT@']
)

bundle_me = []

subdir('CoreLibraries')

subdir('BuildTools')
subdir('Fonts')

subdir('Libraries')
subdir('Tools')

bundled_resources_lib = static_library('bundled-resources', [
    custom_target('bundle',
      output: 'Bundle.cpp',
      command: [
        make_bundle_exe,
        meson.project_build_root() / '@OUTPUT@',
        '@INPUT@',
      ], input: [
        bundle_me
      ]
    ),
    bundle_me
  ], dependencies: [
    ty_dep,
    bundle_dep,
  ])

bundle_dep += declare_dependency(
  link_with: bundled_resources_lib
)

subdir('Plugins')
subdir('src')
