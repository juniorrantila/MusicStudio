
font_sources = [
  './OxaniumLight/Oxanium-Light.ttf',
  './VictorMono-Regular.ttf',
  './iosevka-regular.ttf',
]

bundle_me += resource_header_gen.process(font_sources)
bundle_libs += static_library('Fonts', [
  resource_source_gen.process(font_sources)
])
