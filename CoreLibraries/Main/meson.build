main_lib = static_library('main', [
    'Main.cpp'
  ], dependencies: [
    core_dep,
    ty_dep,
    c_dep,
  ])

main_dep = declare_dependency(
  link_with: main_lib,
  include_directories: '..'
  )
