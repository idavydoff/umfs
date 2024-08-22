#include "glib.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>

#include "../common.h"
#include "../groups.h"
#include "../users.h"

static int rename_user_in_groups(const char *old_name, const char *new_name)
{
    GList *keys = g_hash_table_get_keys(state.groups);
    GList *keys_ptr = keys;

    bool changed = false;
    while (keys_ptr) {
        Group *group = g_hash_table_lookup(state.groups, keys_ptr->data);

        for (int i = 0; i < group->members_count; i++) {
            if (strcmp(group->members[i], old_name) == 0) {

                free(group->members[i]);
                group->members[i] = NULL;

                group->members[i] = strdup(new_name);
                changed = true;
                break;
            }
        }

        keys_ptr = keys_ptr->next;
    }

    g_list_free(keys);

    if (!changed) {
        return 0;
    }

    int save_result = save_groups_to_file();

    if (save_result != 0) {
        return save_result;
    }

    return 0;
}

int umfs_rename(const char *path, const char *new_path, unsigned int flags)
{
    printf("Rename: from %s, to %s\n", path, new_path);

    bool renamed = false;
    char *path_dup = strdup(path);
    char *new_path_dup = strdup(new_path);

    if (startsWith(path, "/users/")) {
        if (strchr(path + strlen("/users/"), '/')) {
            return -EADDRNOTAVAIL;
        }
        if (!startsWith(new_path, "/users/")
            || strchr(new_path + strlen("/users/"), '/')) {
            return -EADDRNOTAVAIL;
        }
        char *old_name = path_dup + strlen("/users/");
        char *new_name = new_path_dup + strlen("/users/");

        pthread_mutex_lock(&state_data_mutex);

        get_users();
        get_groups();

        if (g_hash_table_contains(state.users, new_name)) {
            pthread_mutex_unlock(&state_data_mutex);
            return -EADDRINUSE;
        }

        User *old_user_data = g_hash_table_lookup(state.users, old_name);
        User *new_user_data = copy_user(old_user_data);

        free(new_user_data->name);
        new_user_data->name = strdup(new_name);

        bool is_home_dir = false;
        if (startsWith(new_user_data->dir, "/home")) {
            char tmp[PATH_MAX];
            sprintf(tmp, "/home/%s", new_name);
            free(new_user_data->dir);
            new_user_data->dir = strdup(tmp);
            is_home_dir = true;
        }

        g_hash_table_remove(state.users, old_name);
        g_hash_table_insert(state.users, strdup(new_name), new_user_data);

        int save_result = save_users_to_file();

        if (save_result != 0) {
            return save_result;
        }

        if (is_home_dir) {
            struct stat st;

            char new_home_path[PATH_MAX];
            sprintf(new_home_path, "/home/%s", new_name);

            char old_home_path[PATH_MAX];
            sprintf(old_home_path, "/home/%s", old_name);

            if (stat(old_home_path, &st) == -1) {
                mkdir(new_home_path, 0700);
            } else {
                rename(old_home_path, new_home_path);
            }
        }

        int rename_in_groups_result = rename_user_in_groups(old_name, new_name);

        if (rename_in_groups_result != 0) {
            return rename_in_groups_result;
        }

        pthread_mutex_unlock(&state_data_mutex);

        renamed = true;
    }

    if (startsWith(path, "/groups/")) {
        if (strchr(path + strlen("/groups/"), '/')) {
            return -EADDRNOTAVAIL;
        }
        if (!startsWith(new_path, "/groups/")
            || strchr(new_path + strlen("/groups/"), '/')) {
            return -EADDRNOTAVAIL;
        }

        char *old_name = path_dup + strlen("/groups/");
        char *new_name = new_path_dup + strlen("/groups/");

        pthread_mutex_lock(&state_data_mutex);

        get_users();
        get_groups();

        if (g_hash_table_contains(state.groups, new_name)) {
            pthread_mutex_unlock(&state_data_mutex);
            return -EADDRINUSE;
        }

        Group *old_group_data = g_hash_table_lookup(state.groups, old_name);
        Group *new_group_data = copy_group(old_group_data);

        free(new_group_data->name);
        new_group_data->name = strdup(new_name);

        g_hash_table_remove(state.groups, old_name);
        g_hash_table_insert(state.groups, strdup(new_name), new_group_data);

        int save_groups_result = save_groups_to_file();

        if (save_groups_result != 0) {
            return save_groups_result;
        }

        pthread_mutex_unlock(&state_data_mutex);

        renamed = true;
    }

    free(path_dup);
    free(new_path_dup);

    if (renamed) {
        pthread_mutex_lock(&state_data_mutex);
        get_users();
        get_groups();
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    return -EADDRNOTAVAIL;
}
