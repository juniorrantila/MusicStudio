binary_to_compressed_c_exe = executable('binary-to-compressed-c', 'binary_to_compressed_c.cpp')

executable('plugin-dump', [
    'plugin-dump.cpp',
  ], dependencies: [
    ms_dep,
    cli_dep,
    main_dep,
  ])

executable('sine', [
    'sine.c',
  ], dependencies: [
    soundio_dep,
  ])
