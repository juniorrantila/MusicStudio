shared_library('BareMinimum', [
    'main.cpp'
  ],
  name_prefix: '',
  include_directories: includes,
  link_with: libVst,
  install_dir: '~/.vst'
  )

if get_option('build_windows_vst')
  find_program('sh', required: true)
  find_program('zig', required: true)
  vst = custom_target(
      command: ['Meta/make-windows.sh'],
      input: [
          'Meta/make-windows.sh',
          'main.cpp',
          '../../Libraries/Vst/AudioPlugin.cpp',
          '../../Libraries/Vst/Vst.cpp',
        ],
      output: ['BareMinimum.dll'],
      install:  true,
      install_dir: '~/.vst'
    )
  declare_dependency(sources: vst)
endif
