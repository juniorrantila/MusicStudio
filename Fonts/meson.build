fonts_lib = static_library('Fonts', [
  './Fonts.cpp'
], dependencies: [
  ty_dep,
  fs_dep
])

fonts_dep = declare_dependency(
  link_with: fonts_lib,
  include_directories: '..'
)
