project('MusicStudio', ['cpp', 'c'],
  default_options: [
    'cpp_std=c++20',
    'cpp_eh=none',
    'cpp_rtti=false',
    #'default_library=static'
    # 'prefer_static=true',
    # 'warning_level=3'
    # 'werror=true'
  ])

  # meson.set('CXX', 'zig c++')
find_program('zig', required: true)
cxx = meson.get_compiler('cpp')
assert(cxx.get_id() == 'clang') # Compiler must be zig c++

add_global_arguments('-Wno-c99-designator', language: 'cpp')
add_global_arguments('-Wno-c99-extensions', language: 'cpp')
add_global_arguments('-Wno-gnu-zero-variadic-macro-arguments', language: 'cpp')
add_global_arguments('-Wno-keyword-macro', language: 'cpp')
add_global_arguments('-Wno-gnu-conditional-omitted-operand', language: 'cpp')
add_global_arguments('-Wno-format-pedantic', language: 'cpp')

add_global_arguments('-fdebug-prefix-map=../=', language: 'cpp')
add_global_arguments('-ffile-prefix-map=../=', language: 'cpp')

#subproject('sdl2',
#  default_options: [
#    'default_library=static'
#    # 'warning_level=0',
#    # 'werror=false'
#    ]
#  )

subdir('Tools')
subdir('Fonts')

includes = [include_directories('.')]
includes += include_directories('Dependencies')
subdir('Dependencies')
includes += include_directories('Libraries')
subdir('Libraries')
includes += include_directories('Plugins')
subdir('Plugins')
includes += include_directories('src')
subdir('src')
