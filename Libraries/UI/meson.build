
ui_target_sources = []
ui_target_deps = []

if target_machine.system() == 'darwin'
  add_languages('objcpp')
  ui_target_deps += dependency('cocoa')
  ui_target_sources += 'macos/Window.mm'
else
  ui_target_sources += 'dummy/Window.cpp'
endif

ui_lib = library('UI', [
    ui_target_sources,
    'UI.cpp',
    'FreeGlyph.cpp',
    'SimpleRenderer.cpp',
  ], dependencies: [
    ui_target_deps,
    freetype2_dep,
    glew_dep,
    sdl2_dep,
    ty_dep,
  ])

ui_dep = declare_dependency(
  link_with: ui_lib,
  include_directories: '..'
)
