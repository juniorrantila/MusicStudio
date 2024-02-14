ms_lib = static_library('MS', [
    'Host.cpp',
    'Plugin.cpp',
  ], dependencies: [
    vst_host_dep,
    core_dep,
  ])

ms_dep = declare_dependency(
  link_with: ms_lib,
  include_directories: '..'
)
