core_lib = library('core', [
    'File.cpp',
    'MappedFile.cpp',
    'System.cpp',
    'Library.cpp',
  ], dependencies: [
    ty_dep
  ])

core_dep = declare_dependency(
  link_with: core_lib,
  include_directories: '..'
)
