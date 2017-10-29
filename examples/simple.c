#include <glib.h>
#include <gio/gio.h>
#include "gitlab-client.h"
#include "gitlab-project.h"

void
callback (GObject      *source_object,
          GAsyncResult *res,
          gpointer      user_data)
{
	g_assert (GITLAB_IS_CLIENT (source_object));

	GitlabClient *client = GITLAB_CLIENT (source_object);
	GError *error = NULL;
	GList *projects = gitlab_client_get_projects_finish (client, res, &error);
	if (!projects) {
		g_error ("%s\n", error->message);
		return;
	}

	for (GList *current = projects; current != NULL; current = current->next) {
		GitlabProject *p = current->data;

		g_print ("Name: %s\n", gitlab_project_get_name (p));
	}

	g_list_free_full (projects, g_object_unref);
}

static gboolean
io_callback (GIOChannel   *io,
             GIOCondition  condition,
             gpointer      user_data)
{
	GError *error = NULL;
	gchar in;

	GMainLoop *loop = (GMainLoop*) user_data;

	switch (g_io_channel_read_chars (io, &in, 1, NULL, &error)) {
	case G_IO_STATUS_NORMAL:
		if ('q' == in) {
			g_main_loop_quit (loop);
			return FALSE;
		}
		break;
	}
	return FALSE;
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_print ("Started simple workground\n");
	GitlabClient *client = gitlab_client_new ("https://gitlab.gnome.org/api/v4", "GgnbVSvFS8xj22nFtVg9");

	gitlab_client_get_projects_async (client, callback, NULL, NULL);

	GMainLoop *loop = g_main_loop_new (NULL, FALSE);

	GIOChannel *io = g_io_channel_unix_new (STDIN_FILENO);
	g_io_add_watch (io, G_IO_IN, io_callback, loop);
	g_io_channel_unref (io);

	g_main_loop_run (loop);

	g_object_unref (client);
	return 0;
}
