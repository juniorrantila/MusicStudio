path = meson.current_source_dir()
fonts_sources = [
  embed_gen.process('./OxaniumLight/Oxanium-Light.ttf', preserve_path_from: path),
  embed_gen.process('./VictorMono-Regular.ttf', preserve_path_from: path),
  embed_gen.process('./iosevka-regular.ttf', preserve_path_from: path),
]

fonts_dep = declare_dependency(
  sources: fonts_sources,
  include_directories: '..'
)
