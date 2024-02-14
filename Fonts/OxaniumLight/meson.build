oxanium_light_font = custom_target(
    input: 'Oxanium-Light.ttf',
    output: ['OxaniumLight.h', 'OxaniumLight.cpp'],
    command: [
      binary_to_compressed_c_exe,
      '@INPUT@',
      'oxanium_light',
      '@OUTPUT0@',
      '@OUTPUT1@',
    ]
  )
