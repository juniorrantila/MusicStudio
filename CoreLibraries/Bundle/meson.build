bundle_lib = static_library('bundle', [
  './Bundle.cpp',
], dependencies: [
  ty_dep,
])

bundle_dep = [
    declare_dependency(
    link_with: bundle_lib,
    include_directories: '..',
  )
]
