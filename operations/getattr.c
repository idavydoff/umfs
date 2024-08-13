#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <glib.h>
#include <stdio.h>
#include <limits.h> /* PATH_MAX */

#include "../common.h"
#include "../users.h"
#include "../groups.h"

int umfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;
    printf("Get attr: %s\n", path);

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
        return 0;
    }
    else if (strcmp(path, "/users") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        pthread_mutex_lock(&state_data_mutex);
        stbuf->st_nlink = 1 + g_hash_table_size(state.users);
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }
    else if (strcmp(path, "/groups") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        pthread_mutex_lock(&state_data_mutex);
        stbuf->st_nlink = 1 + g_hash_table_size(state.groups);
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }
    else
    {
        if (startsWith(path, "/users/"))
        {
            pthread_mutex_lock(&state_data_mutex);
            char *name = get_item_name_from_path(path, "/users/");
            struct User *user = g_hash_table_lookup(state.users, name);

            if (user == NULL)
            {
                pthread_mutex_unlock(&state_data_mutex);
                return -ENOENT;
            }

            // /users/<name>
            if (strcmp(path + strlen("/users/"), name) == 0)
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }

            strcat(name, "/");

            char groups_dir_name[NAME_MAX + strlen("/groups")];
            strcpy(groups_dir_name, user->name);
            strcat(groups_dir_name, "/groups");

            // /users/<name>/groups
            if (string_ends_with(path, groups_dir_name))
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }

            // /users/<name>/groups/<group_name>
            if (startsWith(path + strlen("/users/"), groups_dir_name))
            {
                char *group_name = get_path_end(path);
                bool file_valid = false;
                if (g_hash_table_contains(state.groups, group_name))
                {
                    stbuf->st_mode = S_IFLNK | 0666;
                    stbuf->st_nlink = 1;
                    file_valid = true;
                }
                free(group_name);

                pthread_mutex_unlock(&state_data_mutex);
                return file_valid ? 0 : -ENOENT;
            }

            bool file_valid = false;
            char buffer[100];
            // /users/<name>/uid
            if (string_ends_with(path, "/uid") != 0)
            {
                snprintf(buffer, sizeof(buffer), "%d\n", user->uid);
            }
            // /users/<name>/shell
            if (string_ends_with(path, "/shell") != 0)
            {
                snprintf(buffer, sizeof(buffer), "%s\n", user->shell);
            }
            // /users/<name>/dir
            if (string_ends_with(path, "/dir") != 0)
            {
                snprintf(buffer, sizeof(buffer), "%s\n", user->dir);
            }
            // /users/<name>/full_name
            if (string_ends_with(path, "/full_name") != 0)
            {
                snprintf(buffer, sizeof(buffer), "%s\n", user->gecos);
            }
            // /users/<name>/name
            if (string_ends_with(path, "/name") != 0)
            {
                snprintf(buffer, sizeof(buffer), "%s\n", user->name);
            }
            file_valid = string_ends_with(path, "/uid") != 0 ||
                         string_ends_with(path, "/shell") != 0 ||
                         string_ends_with(path, "/dir") != 0 ||
                         string_ends_with(path, "/full_name") != 0 ||
                         string_ends_with(path, "/name") != 0;

            stbuf->st_size = strlen(buffer);
            stbuf->st_mode = S_IFREG | 0666;
            stbuf->st_nlink = 1;

            pthread_mutex_unlock(&state_data_mutex);
            return file_valid ? 0 : -ENOENT;
        }

        if (startsWith(path, "/groups/"))
        {
            pthread_mutex_lock(&state_data_mutex);
            char *name = get_item_name_from_path(path, "/groups/");
            struct Group *group = g_hash_table_lookup(state.groups, name);

            if (group == NULL)
            {
                pthread_mutex_unlock(&state_data_mutex);
                return -ENOENT;
            }

            // /groups/<name>
            if (strcmp(path + strlen("/groups/"), name) == 0)
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }

            char members_dir_name[NAME_MAX + strlen("/members")];
            strcpy(members_dir_name, group->name);
            strcat(members_dir_name, "/members");

            // /groups/<name>/members
            if (string_ends_with(path, members_dir_name))
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }

            // /groups/<name>/members/<member_name>
            if (startsWith(path + strlen("/groups/"), members_dir_name))
            {
                char *user_name = get_path_end(path);
                bool file_valid = false;
                if (g_hash_table_contains(state.users, user_name))
                {
                    stbuf->st_mode = S_IFLNK | 0666;
                    stbuf->st_nlink = 1;
                    file_valid = true;
                }
                free(user_name);

                pthread_mutex_unlock(&state_data_mutex);
                return file_valid ? 0 : -ENOENT;
            }

            bool file_valid = false;
            char buffer[100];
            // /groups/<name>/gid
            if (string_ends_with(path, "/gid") != 0)
            {
                snprintf(buffer, sizeof(buffer), "%d\n", group->gid);
            }
            // /groups/<name>/name
            if (string_ends_with(path, "/name") != 0)
            {
                snprintf(buffer, sizeof(buffer), "%s\n", group->name);
            }

            file_valid = string_ends_with(path, "/gid") != 0 ||
                         string_ends_with(path, "/name") != 0;

            stbuf->st_mode = S_IFREG | 0666;
            stbuf->st_size = strlen(buffer);
            stbuf->st_nlink = 1;

            pthread_mutex_unlock(&state_data_mutex);
            return file_valid ? 0 : -ENOENT;
        }
    }

    pthread_mutex_unlock(&state_data_mutex);
    return -ENOENT;
}
