# Need get_option('prefix') for configuration()
test_execsdir = \
  get_option('prefix') / get_option('libexecdir') / 'installed-tests' / 'ibus'
test_sourcesdir = \
  get_option('prefix') / get_option('datadir') / 'installed-tests' / 'ibus'

ibus_tests = [
  { 'name': 'ibus-bus' },
  { 'name': 'ibus-config' },
  { 'name': 'ibus-configservice' },
  { 'name': 'ibus-factory' },
  { 'name': 'ibus-inputcontext' },
  { 'name': 'ibus-inputcontext-create' },
  { 'name': 'ibus-keynames' },
  { 'name': 'ibus-registry' },
  { 'name': 'ibus-serializable' },
  {
    'name': 'ibus-share',
    'extra_deps': [ dbus_dep ],
  },
  { 'name': 'ibus-util' },
  {
    'name': 'xkb-latin-layouts',
    'is-script': true,
    'dependencies': [ 'display' ],
  },
]

if get_option('engine')
  ibus_tests += { 'name': 'ibus-engine-switch' }
endif

if enable_gtk4 or enable_gtk3
  ibus_tests += {
    'name': 'ibus-compose',
    'extra_deps': [ gtk_dep ],
    'test-install-source': 'ibus-compose-locales.in' ,
    'timeout': 300,
    'dependencies': [ 'display', 'wm' ],
  }

  if libevdev_dep.found() and libudev_dep.found()
    ibus_tests += {
      'name': 'ibus-keypress',
      'extra_sources': [ 'uinput-replay.c' ],
      'extra_deps': [ libevdev_dep, libudev_dep, gtk_dep ],
      'timeout': 60,
      'dependencies': [ 'display', 'wm' ],
    }
  endif
endif

ibus_tests_deps = [
  ibus_dep,
  glib_dep,
  gio_dep,
]

ibus_tests_cflags = [
  '-DIBUS_DISABLE_DEPRECATION_WARNINGS',
  '-DX11_LOCALEDATADIR="@0@"'.format(x11_localedatadir),
]

if libevdev_dep.found() and libudev_dep.found()
  ibus_tests_cflags += '-DUINPUT_REPLAY_DEVICE_EMBED_DATA'
endif

ibus_tests_runner = files(['runtest'])[0]

ibus_tests_env = {
  'top_builddir': build_root,
  'top_srcdir': source_root,
  'builddir': meson.current_build_dir(),
  'srcdir': meson.current_source_dir(),
}

if enable_gtk4
  ibus_tests_env += {
    'TEST_GTK': '1',
    'TEST_GTK_VERSION': '4',
  }
elif enable_gtk3
  ibus_tests_env += {
    'TEST_GTK': '1',
    'TEST_GTK_VERSION': '3',
  }
endif

foreach _test : ibus_tests
  if _test.get('is-script', false)
    test_bin = files([_test['name']])[0]
    if get_option('install-tests')
      install_data(
        test_bin,
        install_dir: test_execsdir,
        install_mode: 'rwxr-xr-x',
      )
    endif
  else
    test_bin = executable(_test['name'],
      files(['@0@.c'.format(_test['name'])] + _test.get('extra_sources', [])),
      dependencies: ibus_tests_deps + _test.get('extra_deps', []),
      c_args: ibus_tests_cflags,
      include_directories: config_h_inc,
      install: get_option('install-tests'),
      install_dir: test_execsdir,
    )
  endif

  timeout = _test.get('timeout', 30)

  test(_test['name'], ibus_tests_runner,
    args: [ test_bin, meson.current_source_dir() ],
    env: environment(ibus_tests_env),
    suite: [ 'lib' ] + _test.get('dependencies', []),
    protocol: 'tap',
    timeout: timeout,
    is_parallel: false,
  )
endforeach

if libevdev_dep.found() and libudev_dep.found()
  uinput_replay_bin = executable('uinput-replay',
    sources: files(['uinput-replay.c']),
    c_args: [ '-DUINPUT_REPLAY_MAIN' ],
    include_directories: config_h_inc,
    dependencies: [ libevdev_dep, libudev_dep, glib_dep ],
    install: get_option('install-tests'),
    install_dir: test_execsdir,
  )
endif

desktop_testing_shells = [
  {
    'name' : 'ibus-desktop-testing-autostart.in',
    'path' : get_option('libexecdir'),
  },
  {
    'name' : 'ibus-desktop-testing-module',
    'path' : get_option('libexecdir'),
  },
  {
    'name' : 'ibus-desktop-testing-runner.in',
    'path' : get_option('bindir'),
  },
]

foreach _desktop_sh : desktop_testing_shells
  installeddir = fs.parent(test_sourcesdir)
  configure_file(
    input: _desktop_sh['name'],
    output: '@BASENAME@',
    configuration: {
      'DATADIR': get_option('prefix') / get_option('datadir'),
      'INSTALLEDDIR': installeddir,
      'LIBEXECDIR': get_option('prefix') / get_option('libexecdir'),
    },
    install_dir: _desktop_sh['path'],
    install_mode: 'rwxr-xr-x',
    install: get_option('install-tests'),
  )
endforeach

if get_option('install-tests')
  install_data(
    'ibus-compose.basic',
    'ibus-compose.basic2',
    'ibus-compose.emoji',
    'ibus-compose.env',
    'ibus-compose.usmin',
    'ibus-compose.usmin2',
    'libinput-test.yml',
    install_dir: test_sourcesdir,
  )

  foreach _test : ibus_tests
    name = _test['name']
    if name == 'ibus-compose'
      name = 'ibus-compose-locales'
    endif

    configure_file(
      input: 'meta.test.in',
      output: '@0@.test'.format(name),
      configuration: {
        'TEST_EXEC': test_execsdir / name,
        'TEST_SRCDIR': test_sourcesdir,
      },
      install_dir: test_sourcesdir,
    )

    if _test.get('test-install-source', '') != ''
      configure_file(
        input: _test.get('test-install-source'),
        output: '@BASENAME@',
        configuration: {
          'SRCDIR': test_sourcesdir,
        },
        install_dir: test_execsdir,
        install_mode: 'rwxr-xr-x',
      )
    endif
  endforeach

  configure_file(
    input: 'ibus-desktop-testing.desktop.in',
    output: 'org.freedesktop.IBus.Desktop.Testing.desktop',
    configuration: {
        'libexecdir': get_option('prefix') / get_option('libexecdir'),
    },
    install_dir: pkgdatadir / 'tests',
  )

  install_data(
    '100-ibus-ci.rules',
    install_dir: pkgdatadir / 'tests',
  )
endif
