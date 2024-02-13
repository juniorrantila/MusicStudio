vst_lib = static_library('vst', [
    'AudioPlugin.cpp',
    'Vst.cpp',
  ], dependencies: [
    ty_dep
  ])

vst_dep = declare_dependency(
  include_directories: '..',
  link_with: vst_lib
)

vst_host_lib = static_library('vst-host', [
    'VstHost.cpp'
  ], dependencies: [
    vst_dep
  ])

vst_host_dep = declare_dependency(
  include_directories: '..',
  link_with: vst_host_lib
)
