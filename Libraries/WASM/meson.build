wasm_lib = static_library('WASM', [
  'Allocator.c',
  'Function.c',
  'VirtualMachine.c',
], dependencies: [
  ty_dep
])

wasm_dep = declare_dependency(
  link_with: wasm_lib,
  include_directories: '..'
)
