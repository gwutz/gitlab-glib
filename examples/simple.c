#include <glib.h>
#include "gitlab-client.h"

gint
main (gint   argc,
      gchar *argv[])
{
	GitlabClient *client = gitlab_client_new ("https://gitlab.gnome.org/api/v4", "GgnbVSvFS8xj22nFtVg9");

	gitlab_client_get_projects (client);

	return 0;
}
