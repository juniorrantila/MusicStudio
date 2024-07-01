make_resource_header_exe = executable('make-resource-header', [
    'make-resource-header.cpp',
  ], dependencies: [
    main_dep,
    cli_dep,
    core_dep,
    ty_dep,
  ])

make_resource_source_exe = executable('make-resource-source', [
    'make-resource-source.cpp',
  ], dependencies: [
    main_dep,
    cli_dep,
    core_dep,
    ty_dep,
  ])

bundle_fs_exe = executable('bundle-fs', [
    'bundle-fs.cpp',
  ], dependencies: [
    main_dep,
    cli_dep,
    core_dep,
    fs_dep,
    ty_dep,
  ])

resource_header_gen = generator(
  make_resource_header_exe,
  output: ['@PLAINNAME@.h'],
  arguments: [
    '../',
    '@INPUT@',
    '@OUTPUT@',
    meson.build_root() / '@BUILD_DIR@',
  ])

resource_source_gen = generator(
  make_resource_source_exe,
  output: ['@PLAINNAME@.c'],
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
