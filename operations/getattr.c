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

            strcat(name, "/");

            char groups_dir_name[NAME_MAX + strlen("/groups")];
            strcpy(groups_dir_name, user->name);
            strcat(groups_dir_name, "/groups");

            if (startsWith(path + strlen("/users/"), name))
            {
                if (strcmp(path + strlen("/users/"), groups_dir_name) == 0)
                {
                    stbuf->st_mode = S_IFDIR | 0755;
                    stbuf->st_nlink = 2;

                    pthread_mutex_unlock(&state_data_mutex);
                    return 0;
                }

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
                if (string_ends_with(path, "/uid") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%d\n", user->uid);
                    stbuf->st_size = strlen(buffer);
                    file_valid = true;
                }
                if (string_ends_with(path, "/shell") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%s\n", user->shell);
                    stbuf->st_size = strlen(buffer);
                    file_valid = true;
                }
                if (string_ends_with(path, "/dir") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%s\n", user->dir);
                    stbuf->st_size = strlen(buffer);
                    file_valid = true;
                }
                if (string_ends_with(path, "/full_name") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%s\n", user->gecos);
                    stbuf->st_size = strlen(buffer);
                    file_valid = true;
                }
                if (string_ends_with(path, "/name") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%s\n", user->name);
                    stbuf->st_size = strlen(buffer);
                    file_valid = true;
                }

                stbuf->st_mode = S_IFREG | 0666;
                stbuf->st_nlink = 1;

                pthread_mutex_unlock(&state_data_mutex);
                return file_valid ? 0 : -ENOENT;
            }
            else
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }
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

            strcat(name, "/");

            char members_dir_name[NAME_MAX + strlen("/members")];
            strcpy(members_dir_name, group->name);
            strcat(members_dir_name, "/members");

            if (startsWith(path + strlen("/groups/"), name))
            {
                if (strcmp(path + strlen("/groups/"), members_dir_name) == 0)
                {
                    stbuf->st_mode = S_IFDIR | 0755;
                    stbuf->st_nlink = 2;

                    pthread_mutex_unlock(&state_data_mutex);
                    return 0;
                }

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
                if (string_ends_with(path, "/gid") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%d\n", group->gid);
                    stbuf->st_size = strlen(buffer);
                    file_valid = true;
                }
                if (string_ends_with(path, "/name") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%s\n", group->name);
                    stbuf->st_size = strlen(buffer);
                    file_valid = true;
                }

                stbuf->st_mode = S_IFREG | 0666;
                stbuf->st_nlink = 1;

                pthread_mutex_unlock(&state_data_mutex);
                return file_valid ? 0 : -ENOENT;
            }
            else
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }
        }
    }

    pthread_mutex_unlock(&state_data_mutex);
    return -ENOENT;
}
