oxanium_light_font = custom_target(
    input: 'OxaniumLight/Oxanium-Light.ttf',
    output: ['OxaniumLight.h', 'OxaniumLight.cpp'],
    command: [
      binary_to_compressed_c, 
      '@INPUT@',
      'oxanium_light',
      '@OUTPUT0@',
      '@OUTPUT1@',
    ]
  )
# declare_dependency(sources: oxanium_light_font)

libFonts = library('Fonts', oxanium_light_font)
