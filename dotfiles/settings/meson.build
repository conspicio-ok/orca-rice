project('orca-settings', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++17', 'warning_level=3', 'buildtype=release'])

gtkmm = dependency('gtkmm-3.0')
layershell = dependency('gtk-layer-shell-0')
pulse = dependency('libpulse')
pulseglib = dependency('libpulse-mainloop-glib')

executable('orca-settings',
  'src/main.cpp',
  'src/radio.cpp',
  'src/volume.cpp',
  'src/window.cpp',
  dependencies: [gtkmm, layershell, pulse, pulseglib],
  install: true)
