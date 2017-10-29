/* gitlab-client.c
 *
 * Copyright (C) 2017 Günther Wutz <info@gunibert.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gitlab-client.h"
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <stdlib.h>
#include <time.h>

struct _GitlabClient
{
	GObject parent_instance;

	gchar *baseurl;
	gchar *token;

	SoupSession *session;
};

G_DEFINE_TYPE (GitlabClient, gitlab_client, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_BASEURL,
	PROP_TOKEN,
	N_PROPS
};

static GParamSpec *properties [N_PROPS];

GitlabClient *
gitlab_client_new (gchar *baseurl,
                   gchar *token)
{
	GitlabClient *client = g_object_new (GITLAB_TYPE_CLIENT,
											 								 "baseurl", baseurl,
											 								 "token", token,
											 								 NULL);
	client->session = soup_session_new ();

	return client;
}

static void
gitlab_client_finalize (GObject *object)
{
	GitlabClient *self = (GitlabClient *)object;

	g_free (self->baseurl);
	g_free (self->token);
	g_object_unref (self->session);

	G_OBJECT_CLASS (gitlab_client_parent_class)->finalize (object);
}

static void
gitlab_client_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	GitlabClient *self = GITLAB_CLIENT (object);

	switch (prop_id)
	  {
		case PROP_BASEURL:
			g_value_set_string (value, self->baseurl);
			break;
		case PROP_TOKEN:
			g_value_set_string (value, self->token);
			break;
	  default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	  }
}

static void
gitlab_client_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	GitlabClient *self = GITLAB_CLIENT (object);

	switch (prop_id)
	  {
		case PROP_BASEURL:
			self->baseurl = g_value_dup_string (value);
			break;
		case PROP_TOKEN:
			self->token = g_value_dup_string (value);
			break;
	  default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	  }
}

static void
gitlab_client_class_init (GitlabClientClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gitlab_client_finalize;
	object_class->get_property = gitlab_client_get_property;
	object_class->set_property = gitlab_client_set_property;

	properties[PROP_BASEURL] =
		g_param_spec_string ("baseurl",
												 "Baseurl",
												 "The url of the gitlab instance",
												 "localhost",
												 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_TOKEN] =
		g_param_spec_string ("token",
												 "Token",
												 "The private access token",
												 "",
												 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gitlab_client_init (GitlabClient *self)
{
}

void
gitlab_client_get_version (GitlabClient  *self,
                           const gchar        **version,
                           const gchar        **revision)
{
	GError *error = NULL;

	gchar *url = g_strconcat (self->baseurl, "/version", NULL);
	SoupMessage *msg = soup_message_new ("GET", url);
	g_free (url);

	soup_message_headers_append (msg->request_headers, "PRIVATE-TOKEN", self->token);

	soup_session_send_message (self->session, msg);

	JsonParser *parser = json_parser_new ();
	json_parser_load_from_data (parser, msg->response_body->data, msg->response_body->length, &error);
	JsonNode *root = json_parser_get_root (parser);
	JsonObject *object = json_node_get_object (root);
	*version = json_object_get_string_member (object, "version");
	*revision = json_object_get_string_member (object, "revision");
}

static GList *
gitlab_client_parse_projects (GInputStream *stream,
                              GCancellable *cancellable,
                              GError       *error)
{
	GList *list = NULL;
	JsonParser *parser = json_parser_new ();

	json_parser_load_from_stream (parser, stream, cancellable, &error);
	JsonNode *root = json_parser_get_root (parser);

	JsonArray *array = json_node_get_array (root);
	for (int i = 0; i < json_array_get_length (array); i++) {
		JsonNode *node = json_array_get_element (array, i);
		JsonObject *object = json_node_get_object (node);

		/* if (json_object_has_member (object, "forked_from_project")) { */
		/* 	continue; */
		/* } */

		gint64 id = json_object_get_int_member (object, "id");
		g_autofree gchar *name = g_strdup (json_object_get_string_member (object, "name_with_namespace"));
		g_autofree gchar *description = g_strdup (json_object_get_string_member (object, "description"));
		g_autofree gchar *avatar = g_strdup (json_object_get_string_member (object, "avatar_url"));

		GitlabProject *p = gitlab_project_new (id, name, description, avatar);
		list = g_list_append (list, p);
	}

	g_object_unref (parser);
	return list;
}

static SoupMessage*
gitlab_client_auth_message (GitlabClient *self,
                            gchar        *url)
{
	SoupMessage *msg = soup_message_new ("GET", url);
	soup_message_headers_append (msg->request_headers, "PRIVATE-TOKEN", self->token);
	return msg;
}

static void
gitlab_client_get_projects_cb (GTask        *task,
                               gpointer      source_object,
                               gpointer      task_data,
                               GCancellable *cancellable)
{
	g_autoptr(GInputStream) stream = NULL;
	GError *error = NULL;
	GList *list = NULL;

	g_assert (GITLAB_IS_CLIENT (source_object));
	g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

	GitlabClient *self = GITLAB_CLIENT (source_object);
	g_autofree gchar *url = g_strconcat (self->baseurl, "/groups/GNOME/projects", NULL);
	SoupMessage *msg = gitlab_client_auth_message (self, url);
	stream = soup_session_send (self->session, msg, cancellable, &error);
	g_object_unref (msg);
	if (!stream && !g_input_stream_close (stream, cancellable, &error)) {
		g_task_return_error (task, error);
	}

	const gchar *pages_str = soup_message_headers_get_one (msg->response_headers, "X-Total-Pages");
	int pages = strtol (pages_str, NULL, 10);

	for (int i = 1; i <= pages; i++)
	  {
			char p[3];
			g_snprintf (p, 3, "%d", i);
			url = g_strconcat (self->baseurl, "/projects", "?page=", p, NULL);

			msg = gitlab_client_auth_message (self, url);
			stream = soup_session_send (self->session, msg, cancellable, &error);
			if (!stream) {
				g_task_return_error (task, error);
			}
			GList *more = gitlab_client_parse_projects (stream, cancellable, error);
			if (!g_input_stream_close (stream, cancellable, &error)) {
				g_task_return_error (task, error);
			}
			g_object_unref (msg);

			if (list == NULL) {
				list = more;
			} else {
				list = g_list_concat (list, more);
			}
	  }

	g_task_return_pointer (task, list, NULL);
}

/**
 * gitlab_client_get_projects_async:
 * @self: a #GitlabClient
 * @callback: the callback for the async operation
 * @cancellable: (nullable): a #Gcancellable, or %NULL
 * @user_data: user defined parameter
 *
 * Asynchronously loads all projects, which are not forked from other projects
 *
 * See also: gitlab_client_get_projects_finish()
 */
void
gitlab_client_get_projects_async (GitlabClient        *self,
                                  GAsyncReadyCallback  callback,
                                  GCancellable        *cancellable,
                                  gpointer             user_data)
{
	g_autoptr (GTask) task = NULL;

	g_assert (GITLAB_IS_CLIENT (self));
	g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

	task = g_task_new (self, cancellable, callback, user_data);

	g_task_run_in_thread (task, gitlab_client_get_projects_cb);
}

GList *
gitlab_client_get_projects_finish (GitlabClient *self,
                                   GAsyncResult  *res,
                                   GError        **error)
{
	g_assert (GITLAB_IS_CLIENT (self));
	g_assert (G_IS_TASK (res));

	return g_task_propagate_pointer (G_TASK(res), error);
}

static void
gitlab_client_get_project_issues_cb (GTask        *task,
                                     gpointer      source_object,
                                     gpointer      task_data,
                                     GCancellable *cancellable)
{
	g_autoptr (GInputStream) stream = NULL;
	GError *error = NULL;
	g_autoptr (SoupMessage) msg = NULL;

	g_assert (GITLAB_IS_CLIENT (source_object));
	g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));
	g_assert (GITLAB_IS_PROJECT (task_data));

	GitlabClient *self = GITLAB_CLIENT (source_object);
	GitlabProject *project = GITLAB_PROJECT (task_data);

	/*g_autofree gchar *url2 = g_strconcat (self->baseurl,
																			 "/projects/",
																			 gitlab_project_get_id(project),
																			 "/issues",
																			 NULL);*/
	g_autofree gchar *url = g_strdup_printf ("%s/projects/%d/issues", self->baseurl, gitlab_project_get_id (project));
	g_print ("URL: %s\n", url);
	msg = gitlab_client_auth_message (self, url);
	/* stream = soup_session_send (self->session, msg, cancellable, &error); */
	/* if (!stream) { */
	/* 	g_task_return_error (task, error); */
	/* } */

	soup_session_send_message (self->session, msg);

	g_print ("%s\n", msg->response_body->data);
}

/**
 * gitlab_client_get_project_issues_async:
 * @self: A #GitlabClient
 * @project: A #GitlabProject
 * @callback: the callback for the async operation
 * @cancellable: (nullable): A #GCancellable, or %NULL
 * @user_data: user defined parameter
 *
 * Asynchronously loads all issues to a specific #GitlabProject
 *
 * See also: gitlab_client_get_project_issues_finish()
 */
void
gitlab_client_get_project_issues_async (GitlabClient        *self,
                                        GitlabProject       *project,
                                        GAsyncReadyCallback  callback,
                                        GCancellable        *cancellable,
                                        gpointer             user_data)
{
	g_autoptr (GTask) task = NULL;

	g_assert (GITLAB_IS_CLIENT (self));
	g_assert (GITLAB_IS_PROJECT (project));
	g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

	task = g_task_new (self, cancellable, callback, NULL);
	g_task_set_task_data (task, project, g_object_unref);

	g_task_run_in_thread (task, gitlab_client_get_project_issues_cb);
}

GList *
gitlab_client_get_project_issues_finish (GitlabClient  *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
	g_assert (GITLAB_IS_CLIENT (self));
	g_assert (G_IS_TASK (res));

	return g_task_propagate_pointer (G_TASK (res), error);
}
