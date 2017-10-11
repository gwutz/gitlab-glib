source_c = [
	'gitlab-client.c',
	'gitlab-project.c'
]

gitlab_include = include_directories('.')

gitlab_lib = library('gitlab_glib',
	source_c,
	dependencies: [gobject_dep, libsoup_dep, json_glib_dep],
	install: true)

pkgconfig = import('pkgconfig')

pkgconfig.generate(
	name: 'gitlab_glib',
	description: 'A GObject based Gitlab client library',
	libraries: gitlab_lib,
	version: '0.1'
	)
