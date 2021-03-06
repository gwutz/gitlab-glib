/* gitlab-project.c
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
#include "gitlab-project.h"

struct _GitlabProject
{
	GObject parent_instance;

	gint id;
	gchar *name;
	gchar *description;
	gchar *avatar;
	gchar *http_url_to_repo;
};

G_DEFINE_TYPE (GitlabProject, gitlab_project, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_ID,
	PROP_NAME,
	PROP_DESCRIPTION,
	PROP_AVATAR,
	PROP_HTTP_URL_TO_REPO,
	N_PROPS
};

static GParamSpec *properties [N_PROPS];

GitlabProject *
gitlab_project_new_from_node (JsonNode *node)
{
	JsonObject *object = json_node_get_object (node);

	gint id = json_object_get_int_member (object, "id");
	g_autofree gchar *name = g_strdup (json_object_get_string_member (object, "name_with_namespace"));
	g_autofree gchar *description = g_strdup (json_object_get_string_member (object, "description"));
	g_autofree gchar *avatar = g_strdup (json_object_get_string_member (object, "avatar_url"));
	g_autofree gchar *http_url_to_repo = g_strdup (json_object_get_string_member (object, "http_url_to_repo"));

	GitlabProject *self = g_object_new (GITLAB_TYPE_PROJECT,
											 "id", id,
											 "name", name,
											 "description", description,
											 "avatar", avatar,
											 "http-url-to-repo", http_url_to_repo,
											 NULL);

	return self;
}

GitlabProject *
gitlab_project_new (int id, gchar *name, gchar *description, gchar *avatar)
{
	return g_object_new (GITLAB_TYPE_PROJECT,
											 "id", id,
											 "name", name,
											 "description", description,
											 "avatar", avatar,
											 NULL);
}

static void
gitlab_project_finalize (GObject *object)
{
	GitlabProject *self = (GitlabProject *)object;

	g_free (self->name);
	g_free (self->description);
	g_free (self->avatar);
	g_free (self->http_url_to_repo);

	G_OBJECT_CLASS (gitlab_project_parent_class)->finalize (object);
}

static void
gitlab_project_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
	GitlabProject *self = GITLAB_PROJECT (object);

	switch (prop_id)
	  {
		case PROP_ID:
			g_value_set_int (value, self->id);
			break;
		case PROP_NAME:
			g_value_set_string (value, self->name);
			break;
		case PROP_DESCRIPTION:
			g_value_set_string (value, self->description);
			break;
		case PROP_AVATAR:
			g_value_set_string (value, self->avatar);
			break;
		case PROP_HTTP_URL_TO_REPO:
			g_value_set_string (value, self->http_url_to_repo);
			break;
	  default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	  }
}

static void
gitlab_project_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
	GitlabProject *self = GITLAB_PROJECT (object);

	switch (prop_id)
	  {
		case PROP_ID:
			self->id = g_value_get_int (value);
			break;
		case PROP_NAME:
			self->name = g_value_dup_string (value);
			break;
		case PROP_DESCRIPTION:
			self->description = g_value_dup_string (value);
			break;
		case PROP_AVATAR:
			self->avatar = g_value_dup_string (value);
			break;
		case PROP_HTTP_URL_TO_REPO:
			self->http_url_to_repo = g_value_dup_string (value);
			break;
	  default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	  }
}

static void
gitlab_project_class_init (GitlabProjectClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gitlab_project_finalize;
	object_class->get_property = gitlab_project_get_property;
	object_class->set_property = gitlab_project_set_property;

	properties[PROP_ID] =
		g_param_spec_int ("id", "Id", "The unique id of the project", 0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_NAME] =
		g_param_spec_string ("name", "Name", "The name of the project", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_DESCRIPTION] =
		g_param_spec_string ("description", "Description", "The description of the project", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_AVATAR] =
		g_param_spec_string ("avatar", "Avatar", "The url of the avatar of the project", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	properties[PROP_HTTP_URL_TO_REPO] =
		g_param_spec_string ("http-url-to-repo", "Http-url-to-repo", "The http url of the repository", "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPS, properties);

}

static void
gitlab_project_init (GitlabProject *self)
{
}

gint
gitlab_project_get_id (GitlabProject *self)
{
	return self->id;
}

gchar *
gitlab_project_get_name (GitlabProject *self)
{
	return self->name;
}

gchar *
gitlab_project_get_description (GitlabProject *self)
{
	return self->description;
}

gchar *
gitlab_project_get_avatar (GitlabProject *self)
{
	return self->avatar;
}

gchar *
gitlab_project_get_http_url_to_repo (GitlabProject *self)
{
	return self->http_url_to_repo;
}
