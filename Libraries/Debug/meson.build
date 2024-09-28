debug_lib = static_library('debug', [
  'Instrumentation.cpp'
], cpp_args: [
  '-ibuiltininc'
])

debug_dep = declare_dependency(
  link_with: debug_lib,
  include_directories: '..'
)
