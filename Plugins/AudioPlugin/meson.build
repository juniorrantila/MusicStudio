shared_library('AudioPlugin', [
    'AudioPlugin.cpp',
    'main.cpp',
  ],
  name_prefix: '',
  include_directories: includes,
  link_with: [libImgui, libImguiKnobs, libFonts, libVst],
  dependencies: sdl2_dep
  )


if get_option('build_windows_vst')
  find_program('sh', required: true)
  find_program('zig', required: true)
  vst = custom_target(
      command: ['Meta/make-windows.sh'],
      input: [
          'Meta/make-windows.sh',
          'AudioPlugin.cpp',
          'main.cpp',
          oxanium_light_font,
          '../../Libraries/Vst/AudioPlugin.cpp',
          '../../Libraries/Vst/Vst.cpp',
          '../../Dependencies/imgui-knobs/imgui-knobs.cpp',
          '../../Dependencies/imgui/backends/imgui_impl_sdl.cpp',
          '../../Dependencies/imgui/backends/imgui_impl_sdlrenderer.cpp',
          '../../Dependencies/imgui/imgui.cpp',
          '../../Dependencies/imgui/imgui_draw.cpp',
          '../../Dependencies/imgui/imgui_tables.cpp',
          '../../Dependencies/imgui/imgui_widgets.cpp',
          '../../Dependencies/SDL2Windows/SDL2/lib/libSDL2.a',
        ],
      output: ['AudioPlugin.dll'],
      install:  true,
      install_dir: '~/.vst'
    )
  declare_dependency(sources: vst)
endif
