fs_lib = static_library('fs', [
  './Bundle.cpp',
], dependencies: [
  ty_dep,
  zip_dep
])

fs_dep = declare_dependency(
  link_with: fs_lib,
  include_directories: '..',
)

fs_bundle_dep = []
