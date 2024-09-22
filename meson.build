project('MusicStudio', ['cpp', 'c'],
  default_options: [
    'cpp_std=c++20',
    'c_std=c2x',
    'cpp_eh=none',
    'cpp_rtti=false',
    'default_library=static',
    'prefer_static=true',
    'warning_level=3',
    'buildtype=release',
    'strip=true',
  ])

cpp = meson.get_compiler('cpp')
if not cpp.compiles('''
#if __has_embed(__FILE__)
#else
#error "no #embed support"
#endif
int main(){}
''')
  error('compiler needs #embed support')
endif

add_project_arguments(cpp.get_supported_arguments([
  '-Werror=return-type',
  '-Werror=switch',
  '-Werror',
  '-Wformat=2',
  '-Wimplicit-fallthrough',
  '-Wmissing-declarations',
  '-Wmissing-prototypes',
  '-Wno-c99-designator',
  '-Wno-c99-extensions',
  '-Wno-expansion-to-defined',
  '-Wno-format-pedantic',
  '-Wno-gnu-anonymous-struct',
  '-Wno-gnu-designator',
  '-Wno-gnu-case-range',
  '-Wno-gnu-conditional-omitted-operand',
  '-Wno-gnu-statement-expression',
  '-Wno-gnu-zero-variadic-macro-arguments',
  '-Wno-implicit-const-int-float-conversion',
  '-Wno-invalid-offsetof',
  '-Wno-keyword-macro',
  '-Wno-literal-suffix',
  '-Wno-nested-anon-types',
  '-Wno-unknown-warning-option',
  '-Wno-unused-command-line-argument',
  '-Wno-user-defined-literals',
  '-Wsuggest-override',
  '-fstrict-flex-arrays=2',
  '-Wno-c23-extensions',
]), language: ['objcpp', 'cpp', 'c'])

add_project_arguments(cpp.get_supported_arguments([
  '-Wcast-qual',
]), language: ['objcpp', 'cpp'])

if target_machine.system() == 'darwin'
  add_languages('objcpp')
  add_project_arguments([
    '-DGL_SILENCE_DEPRECATION=1',
    '-nostdinc++',
  ], language: ['objcpp', 'cpp', 'c'])
  add_project_arguments(cpp.get_supported_arguments([
    '-fobjc-arc',
  ]), language: ['objcpp', 'objc'])
endif

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

subdir('CoreLibraries')
subdir('Fonts')
subdir('Libraries')
subdir('MusicStudio')
