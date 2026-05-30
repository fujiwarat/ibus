ibus_x11_sources = files(
  'gdk-private.c',
  'main.c',
)

ibus_x11_deps = [
  ibus_dep,
  libimdkit_dep,
  x11_dep,
  xfixes_dep,
]

if enable_gtk3
  ibus_x11_deps += gtk3_dep
else
  ibus_x11_deps += gtk2_dep
endif

ibus_x11_module = executable('ibus-x11',
  ibus_x11_sources,
  dependencies: ibus_x11_deps,
  include_directories: config_h_inc,
  install: true,
  install_dir: get_option('libexecdir'),
)

configure_file(
  input: '10-ibus-x11.in',
  output: '@BASENAME@',
  configuration: {
    'libexecdir': get_option('prefix') / get_option('libexecdir'),
  },
  install: true,
  install_dir: get_option('sysconfdir') / 'xdg' / 'Xwayland-session.d',
  install_mode: 'rwxr-xr-x',
)
