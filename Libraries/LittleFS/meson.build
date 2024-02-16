littlefs_lib = static_library('LittleFS', [
    'LittleFS.cpp',
    'Util.cpp',
  ])

littlefs_dep = declare_dependency(
  link_with: littlefs_lib,
  include_directories: '..'
)
