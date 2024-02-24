rexim_platform_sources = []
if target_machine.system() == 'windows'
  rexim_platform_sources += './minirent.cpp'
endif

rexim_lib = static_library('Rexim', [
    'Arena.cpp',
    'File.cpp',
    'StringView.cpp',
    'Util.cpp',
    rexim_platform_sources,
  ], dependencies: [
    ty_dep,
    c_dep,
  ])

rexim_dep = declare_dependency(
  link_with: rexim_lib,
  include_directories: '..'
)
