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

executable('vst-run-gui', [
    'vst-run-gui.cpp',
  ], dependencies: [
    ms_dep,
    cli_dep,
    main_dep,
    ui_dep,
  ])

executable('make-project', [
  'make-project.cpp',
  ], dependencies: [
    littlefs_dep,
    ty_dep,
    main_dep,
    cli_dep,
  ])

executable('read-project', [
  'read-project.cpp',
  ], dependencies: [
    littlefs_dep,
    ty_dep,
    main_dep,
    cli_dep,
  ])
