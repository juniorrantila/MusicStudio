library_lib = library('Library', [
    'Library.c',
  ], dependencies: [
    ty_dep,
  ])

library_dep = declare_dependency(
  link_with: library_lib,
  include_directories: '..'
  )
