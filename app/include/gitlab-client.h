/* gitlab-client.h
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
#include <gio/gio.h>
#include "gitlab-project.h"

G_BEGIN_DECLS

#define GITLAB_TYPE_CLIENT (gitlab_client_get_type())

G_DECLARE_FINAL_TYPE (GitlabClient, gitlab_client, GITLAB, CLIENT, GObject)

GitlabClient *gitlab_client_new (gchar *baseurl, gchar *token);
void gitlab_client_get_version (GitlabClient  *self,
                                const gchar  **version,
                                const gchar  **revision);
void gitlab_client_get_projects_async (GitlabClient        *self,
                                       GAsyncReadyCallback  callback,
                                       GCancellable        *cancellable,
                                       gpointer             user_data);
GList *gitlab_client_get_projects_finish (GitlabClient *self,
                                          GAsyncResult  *res,
                                          GError        **error);
void gitlab_client_get_project_issues_async (GitlabClient        *self,
                                             GitlabProject       *project,
                                             GAsyncReadyCallback  callback,
                                             GCancellable        *cancellable);
GList *gitlab_client_get_project_issues_finish (GitlabClient  *self,
                                                GAsyncResult  *res,
                                                GError       **error);
G_END_DECLS
