make_resource_exe = executable('make-resource', [
    'make-resource.cpp',
  ], dependencies: [
    main_dep,
    cli_dep,
    core_dep,
    ty_dep,
  ])

resource_gen = generator(
  make_resource_exe,
  output: ['@PLAINNAME@.h'],
  arguments: [
    '../',
    '@INPUT@',
    '@OUTPUT@',
    meson.build_root() / '@BUILD_DIR@',
  ])

make_bundle_exe = executable('make-bundle', [
    'make-bundle.cpp',
  ], dependencies: [
    main_dep,
    cli_dep,
    core_dep,
    ty_dep,
  ])
