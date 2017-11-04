public_headers = [
	'gitlab.h',
	'gitlab-client.h',
	'gitlab-project.h'
]

source_c = [
	public_headers,
	'gitlab-client.c',
	'gitlab-project.c'
]

gitlab_include = include_directories('.')

gitlab_lib = library('gitlab-glib',
	source_c,
	dependencies: [gobject_dep, libsoup_dep, json_glib_dep],
	link_depends: 'gitlab.map',
	install: true)

pkgconfig = import('pkgconfig')

pkgconfig.generate(
	name: 'gitlab-glib',
	description: 'A GObject based Gitlab client library',
	libraries: gitlab_lib,
	subdirs: 'gitlab-glib',
	version: '0.1'
	)

install_headers(
	public_headers,
	subdir: 'gitlab-glib'
)
