ui_view_lib = library('UIView', [
    'View.cpp',
    'Text.cpp',
    'Button.cpp',
    'List.cpp',
    'Box.cpp',
  ], dependencies: [
    ui_dep,
    freetype2_dep,
    ty_dep,
    rexim_dep,
  ])

ui_view_dep = declare_dependency(
  link_with: ui_view_lib,
  include_directories: '..'
)
