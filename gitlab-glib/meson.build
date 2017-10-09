source_c = [
	'gitlab-client.c',
	'gitlab-project.c'
]

gitlab_include = include_directories('.')

gitlab_lib = library('gitlab',
	source_c,
	dependencies: [gobject_dep, libsoup_dep, json_glib_dep],
	install: true)
