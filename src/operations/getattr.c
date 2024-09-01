#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <glib.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>

#include "../groups.h"
#include "../umfs.h"
#include "../users.h"
#include "../utils.h"

int umfs_getattr(
    const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;
    printf("Get attr: %s\n", path);

    memset(stbuf, 0, sizeof(struct stat));

    pthread_mutex_lock(&state_data_mutex);
    if (state.fake_file_path != NULL) {
        if (strcmp(path, state.fake_file_path) == 0) {
            stbuf->st_size = 1;
            stbuf->st_mode = S_IFREG | 0666;
            stbuf->st_nlink = 1;
            pthread_mutex_unlock(&state_data_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&state_data_mutex);

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0555;
        stbuf->st_nlink = 4;
        return 0;
    } else if (strcmp(path, "/users") == 0) {
        stbuf->st_mode = S_IFDIR | 0775;
        pthread_mutex_lock(&state_data_mutex);
        stbuf->st_nlink = 2 + g_hash_table_size(state.users);
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    } else if (strcmp(path, "/groups") == 0) {
        stbuf->st_mode = S_IFDIR | 0775;
        pthread_mutex_lock(&state_data_mutex);
        stbuf->st_nlink = 2 + g_hash_table_size(state.groups);
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    } else {
        if (startsWith(path, "/users/")) {
            pthread_mutex_lock(&state_data_mutex);
            char *name = get_item_name_from_path(path, "/users/");
            struct User *user = g_hash_table_lookup(state.users, name);

            if (user == NULL) {
                pthread_mutex_unlock(&state_data_mutex);
                return -ENOENT;
            }

            // /users/<name>
            if (strcmp(path + strlen("/users/"), name) == 0) {
                if (user->privileged) {
                    stbuf->st_mode = S_IFDIR | S_ISVTX | 0755;
                } else {
                    stbuf->st_mode = S_IFDIR | 0755;
                }
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }

            strcat(name, "/");

            char groups_dir_name[NAME_MAX + strlen("/groups")];
            strcpy(groups_dir_name, user->name);
            strcat(groups_dir_name, "/groups");

            // /users/<name>/groups
            if (string_ends_with(path, groups_dir_name)) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2 + user->groups_count;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }

            // /users/<name>/groups/<group_name>
            if (startsWith(path + strlen("/users/"), groups_dir_name)) {
                char *group_name = get_path_end(path);
                bool file_valid = false;

                Group *group = g_hash_table_lookup(state.groups, group_name);

                if (group) {
                    if (group->gid == user->gid) {
                        stbuf->st_size = 1;
                    } else {
                        stbuf->st_size = 0;
                    }
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
            // /users/<name>/id
            if (string_ends_with(path, "/id") != 0) {
                snprintf(buffer, sizeof(buffer), "%d\n", user->uid);
            }
            // /users/<name>/primary_group
            if (string_ends_with(path, "/primary_group") != 0) {
                char *primary_group_name = get_user_primary_group_name(user);
                snprintf(buffer, sizeof(buffer), "%s\n", primary_group_name);
                free(primary_group_name);
            }
            // /users/<name>/shell
            if (string_ends_with(path, "/shell") != 0) {
                snprintf(buffer, sizeof(buffer), "%s\n", user->shell);
            }
            // /users/<name>/directory
            if (string_ends_with(path, "/directory") != 0) {
                snprintf(buffer, sizeof(buffer), "%s\n", user->dir);
            }
            // /users/<name>/full_name
            if (string_ends_with(path, "/full_name") != 0) {
                snprintf(buffer, sizeof(buffer), "%s\n", user->gecos);
            }
            // /users/<name>/ssh_authorized_keys
            if ((string_ends_with(path, "/ssh_authorized_keys") != 0)
                && user->is_home_dir) {
                char home_ssh_dir[PATH_MAX];
                snprintf(home_ssh_dir, PATH_MAX, "%s/.ssh", user->dir);

                struct stat ssh_dir_stat;
                int stat_res = stat(home_ssh_dir, &ssh_dir_stat);

                if (stat_res != 0) {
                    stbuf->st_size = 0;
                }

                char home_authorized_keys_file[PATH_MAX];
                snprintf(home_authorized_keys_file, PATH_MAX,
                    "%s/.ssh/authorized_keys", user->dir);

                struct stat ssh_authorized_keys_file_stat;
                stat_res = stat(
                    home_authorized_keys_file, &ssh_authorized_keys_file_stat);

                if (stat_res != 0) {
                    stbuf->st_size = 0;
                } else {
                    stbuf->st_size = ssh_authorized_keys_file_stat.st_size;
                }
            }

            file_valid = string_ends_with(path, "/id") != 0
                || string_ends_with(path, "/shell") != 0
                || string_ends_with(path, "/directory") != 0
                || string_ends_with(path, "/primary_group") != 0
                || string_ends_with(path, "/full_name") != 0
                || (user->is_home_dir
                        ? string_ends_with(path, "/ssh_authorized_keys") != 0
                        : false);

            if (!((string_ends_with(path, "/ssh_authorized_keys") != 0)
                    && user->is_home_dir)) {
                stbuf->st_size = strlen(buffer);
            }
            stbuf->st_mode = S_IFREG | 0664;
            stbuf->st_nlink = 1;

            pthread_mutex_unlock(&state_data_mutex);
            return file_valid ? 0 : -ENOENT;
        }

        if (startsWith(path, "/groups/")) {
            pthread_mutex_lock(&state_data_mutex);
            char *name = get_item_name_from_path(path, "/groups/");
            struct Group *group = g_hash_table_lookup(state.groups, name);

            if (group == NULL) {
                pthread_mutex_unlock(&state_data_mutex);
                return -ENOENT;
            }

            // /groups/<name>
            if (strcmp(path + strlen("/groups/"), name) == 0) {
                if (group->primary_for_some_users) {
                    stbuf->st_mode = S_IFDIR | S_ISVTX | 0755;
                } else {
                    stbuf->st_mode = S_IFDIR | 0755;
                }
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }

            char members_dir_name[NAME_MAX + strlen("/members")];
            strcpy(members_dir_name, group->name);
            strcat(members_dir_name, "/members");

            // /groups/<name>/members
            if (string_ends_with(path, members_dir_name)) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2 + group->members_count;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }

            // /groups/<name>/members/<member_name>
            if (startsWith(path + strlen("/groups/"), members_dir_name)) {
                char *user_name = get_path_end(path);

                bool file_valid = false;
                bool user_in_members = false;

                for (short i = 0; i < group->members_count; i++) {
                    if (strcmp(group->members[i], user_name) == 0) {
                        user_in_members = true;
                        break;
                    }
                }

                User *user = g_hash_table_lookup(state.users, user_name);
                free(user_name);

                if (user != NULL && user_in_members) {
                    if (user->gid == group->gid) {
                        stbuf->st_size = 1;
                    } else {
                        stbuf->st_size = 0;
                    }
                    stbuf->st_mode = S_IFLNK | 0666;
                    stbuf->st_nlink = 1;
                    file_valid = true;
                }

                pthread_mutex_unlock(&state_data_mutex);
                return file_valid ? 0 : -ENOENT;
            }

            bool file_valid = false;
            char buffer[100];
            // /groups/<name>/id
            if (string_ends_with(path, "/id") != 0) {
                snprintf(buffer, sizeof(buffer), "%d\n", group->gid);
            }

            file_valid = string_ends_with(path, "/id") != 0;

            stbuf->st_mode = S_IFREG | 0664;
            stbuf->st_size = strlen(buffer);
            stbuf->st_nlink = 1;

            pthread_mutex_unlock(&state_data_mutex);
            return file_valid ? 0 : -ENOENT;
        }
    }

    pthread_mutex_unlock(&state_data_mutex);
    return -ENOENT;
}
