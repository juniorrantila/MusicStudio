ui_shader_sources = [
  './simple.vert',
  './simple_color.frag',
  './simple_image.frag',
  './simple_text.frag',
  './simple_epic.frag',
]

bundle_me += resource_header_gen.process(ui_shader_sources)

bundle_libs += static_library('ui-shaders', [
  resource_source_gen.process(ui_shader_sources)
])
