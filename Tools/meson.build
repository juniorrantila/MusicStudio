binary_to_compressed_c_exe = executable('binary-to-compressed-c', 'binary_to_compressed_c.cpp')

executable('vst-dump', [
    'vst-dump.cpp',
  ], dependencies: [
    ms_dep,
    cli_dep,
    main_dep,
  ])

executable('sine', [
    'sine.cpp',
  ], dependencies: [
    main_dep,
    soundio_dep,
    ty_dep,
    cli_dep,
  ])

executable('list-devices', [
    'list-devices.cpp',
  ], dependencies: [
    main_dep,
    soundio_dep,
    ty_dep,
    cli_dep,
  ])

if target_machine.system() == 'darwin'
  add_languages('objcpp')
  cocoa_dep = dependency('cocoa')
  executable('vst-run-gui', [
      'vst-run-gui.mm',
    ], dependencies: [
      cocoa_dep,
      ms_dep,
      cli_dep,
      main_dep,
    ])
endif
