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
#include "gitlab-project.h"
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <stdlib.h>

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

GList *
gitlab_client_get_projects_part(SoupMessage *msg)
{
	GError *error = NULL;
	GList *list = NULL;
	JsonParser *parser = json_parser_new ();

	json_parser_load_from_data (parser, msg->response_body->data, msg->response_body->length, &error);
	JsonNode *root = json_parser_get_root (parser);

	JsonArray *array = json_node_get_array (root);
	for (int i = 0; i < json_array_get_length (array); i++) {
		JsonNode *node = json_array_get_element (array, i);
		JsonObject *object = json_node_get_object (node);

		const gchar *name = json_object_get_string_member (object, "name_with_namespace");
		const gchar *description = json_object_get_string_member (object, "description");

		if (!json_object_has_member (object, "forked_from_project")) {
			GitlabProject *p = gitlab_project_new (name, description);
			list = g_list_append (list, p);
		}
	}

	g_object_unref (parser);
	return list;
}

GList *
gitlab_client_get_projects (GitlabClient *self)
{
	GList *list = NULL;

	gchar *url = g_strconcat (self->baseurl, "/projects", NULL);
	SoupMessage *msg = soup_message_new ("GET", url);
	g_free (url);

	soup_message_headers_append (msg->request_headers, "PRIVATE-TOKEN", self->token);

	soup_session_send_message (self->session, msg);

	list = gitlab_client_get_projects_part (msg);
	const gchar *pages_str = soup_message_headers_get_one (msg->response_headers, "X-Total-Pages");
	int pages = strtol (pages_str, NULL, 10);
	const gchar *current_page_str = soup_message_headers_get_one (msg->response_headers, "X-Page");
	int current_page = strtol (current_page_str, NULL, 10);

	for (int i = ++current_page; i <= pages; i++) {
		char p[3];
		g_snprintf (p, 3, "%d", i);
		gchar *url = g_strconcat (self->baseurl, "/projects", "?page=", p, NULL);

		msg = soup_message_new ("GET", url);
		g_free (url);

		soup_message_headers_append (msg->request_headers, "PRIVATE-TOKEN", self->token);

		soup_session_send_message (self->session, msg);

		GList *more = gitlab_client_get_projects_part (msg);
		list = g_list_concat (list, more);
	}

	return list;
}
