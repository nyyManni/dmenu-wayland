project(
  'dmenu-wl',
  'c',
  version: '4.2.1',
  license: 'MIT',
  default_options: ['c_std=c99']
)

add_project_arguments(
  [
    '-pedantic',
    '-Wall',
    '-D_DEFAULT_SOURCE',
    '-DVERSION="@0@"'.format(meson.project_version()),
  ],
  language : 'c'
)

cc = meson.get_compiler('c')

cairo               = dependency('cairo')
glib                = dependency('glib-2.0')
gobject             = dependency('gobject-2.0')
pango               = dependency('pango')
pangocairo          = dependency('pangocairo')
rt                  = cc.find_library('rt')
wayland_client      = dependency('wayland-client')
wayland_protos      = dependency('wayland-protocols')
wayland_scanner_dep = dependency('wayland-scanner', native: true)
wayland_scanner     = find_program(
  wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner')
)
xkbcommon = dependency('xkbcommon')

wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')

protocols = [
  [wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
  [wl_protocol_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml'],
  'wlr-layer-shell-unstable-v1.xml'
]

protos_src = []
protos_headers = []

foreach p : protocols
  xml = join_paths(p)
  protos_src += custom_target(
    xml.underscorify() + '_client_c',
    input: xml,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'public-code', '@INPUT@', '@OUTPUT@'],
  )
  protos_headers += custom_target(
    xml.underscorify() + '_client_h',
    input: xml,
    output: '@BASENAME@-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
endforeach

dmenu_deps = [
  cairo,
  glib,
  gobject,
  pango,
  pangocairo,
  rt,
  wayland_client,
  xkbcommon
]

dmenu_src = ['dmenu.c', 'draw.c'] + protos_src + protos_headers

executable('dmenu-wl', dmenu_src, dependencies: dmenu_deps, install: true)

executable('dmenu-wl_path', 'dmenu_path.c', install: true)

install_data('dmenu-wl_run', install_dir: get_option('bindir'))

install_man('dmenu-wl.1')

