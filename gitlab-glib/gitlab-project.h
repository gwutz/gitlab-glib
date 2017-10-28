/* gitlab-project.h
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
#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define GITLAB_TYPE_PROJECT (gitlab_project_get_type())

G_DECLARE_FINAL_TYPE (GitlabProject, gitlab_project, GITLAB, PROJECT, GObject)

GitlabProject *gitlab_project_new (gchar *name, gchar *description, gchar *avatar);
gint   gitlab_project_get_id (GitlabProject *self);
gchar *gitlab_project_get_name (GitlabProject *self);
gchar *gitlab_project_get_description (GitlabProject *self);
gchar *gitlab_project_get_avatar (GitlabProject *self);

G_END_DECLS
