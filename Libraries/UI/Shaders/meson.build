path = meson.current_source_dir() / '..'
ui_shaders_sources = [
  embed_gen.process('./simple.vert', preserve_path_from: path),
  embed_gen.process('./simple_color.frag', preserve_path_from: path),
  embed_gen.process('./simple_image.frag', preserve_path_from: path),
  embed_gen.process('./simple_text.frag', preserve_path_from: path),
  embed_gen.process('./simple_epic.frag', preserve_path_from: path),
]

ui_shaders_dep = declare_dependency(
  sources: ui_shaders_sources,
  include_directories: '..'
  )
