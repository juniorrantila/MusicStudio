subdir('Shaders')

ui_target_sources = []
ui_target_deps = []

if target_machine.system() == 'darwin'
  ui_target_deps += dependency('appkit', static: true)
  ui_target_sources += 'macos/Window.mm'
  ui_target_sources += 'macos/Application.mm'
  ui_target_sources += 'macos/UIApp.mm'
else
  ui_target_sources += 'dummy/Window.cpp'
  ui_target_sources += 'dummy/Application.cpp'
endif

ui_lib = library('UI', [
    ui_target_sources,
    'UI.cpp',
    'FreeGlyph.cpp',
    'SimpleRenderer.cpp',
  ], dependencies: [
    ui_target_deps,
    freetype2_dep,
    opengl_dep,
    ty_dep,
    fs_dep,
    fs_bundle_dep,
    rexim_dep,
  ])

ui_dep = declare_dependency(
  link_with: ui_lib,
  include_directories: '..'
)
