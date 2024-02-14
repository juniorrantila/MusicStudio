synth_lib = library('synth', [
  'synth.c',
], dependencies: [
  audio_dep,
])

synth_dep = declare_dependency(
  link_with: synth_lib,
  include_directories: '..'
)
