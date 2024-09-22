cli_lib = library('cli', [
    'ArgumentParser.cpp',
    'ArgumentParserError.cpp'
  ],
  dependencies: [
    core_dep, 
    ty_dep,
  ])

cli_dep = declare_dependency(
  link_with: cli_lib,
  include_directories: '..'
  )
