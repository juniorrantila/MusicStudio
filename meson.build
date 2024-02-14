project('MusicStudio', 'cpp',
  default_options: [
    'cpp_std=c++20',
    'cpp_eh=none',
    'cpp_rtti=false',
    'default_library=static',
    'prefer_static=true',
    'warning_level=3'
  ])

add_global_arguments([
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
], language: 'cpp')

subdir('Libraries')

subdir('Tools')
subdir('Fonts')

subdir('Plugins')

subdir('src')
