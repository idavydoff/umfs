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

int umfs_rmdir(const char *path)
{
    printf("Rmdir: %s\n", path);

    bool deleted = false;

    if (startsWith(path, "/users/")) {
        if (strchr(path + strlen("/users/"), '/'))
            return -EINVAL;

        pthread_mutex_lock(&state_data_mutex);

        char *name = get_path_end(path);
        User *user = g_hash_table_lookup(state.users, name);

        GList *groups_keys = g_hash_table_get_keys(state.groups);
        GList *groups_keys_ptr = groups_keys;

        bool groups_modified = false;
        while (groups_keys_ptr) {
            Group *ptr_group
                = g_hash_table_lookup(state.groups, groups_keys_ptr->data);

            if (ptr_group->gid == user->gid) {
                if (ptr_group->members_count == 1) {
                    g_hash_table_remove(state.groups, groups_keys_ptr->data);
                    groups_modified = true;
                }
                groups_keys_ptr = groups_keys_ptr->next;
                continue;
            }

            short deleted_from_members_count = delete_from_dynamic_string_array(
                &ptr_group->members, ptr_group->members_count, name);

            if (deleted_from_members_count) {
                groups_modified = true;
                ptr_group->members_count -= deleted_from_members_count;
            }

            groups_keys_ptr = groups_keys_ptr->next;
        }

        g_list_free(groups_keys);

        if (groups_modified) {
            int save_res = save_groups_to_file();
            if (save_res != 0) {
                pthread_mutex_unlock(&state_data_mutex);
                return save_res;
            }
        }

        g_hash_table_remove(state.users, name);

        int save_res = save_users_to_file();
        if (save_res != 0) {
            pthread_mutex_unlock(&state_data_mutex);
            return save_res;
        }

        deleted = true;
    }

    if (startsWith(path, "/groups/")) {
        if (strchr(path + strlen("/groups/"), '/'))
            return -EINVAL;

        char *name = get_path_end(path);

        pthread_mutex_lock(&state_data_mutex);

        Group *group = g_hash_table_lookup(state.groups, name);

        // Нельзя удалять группы, которые являются первичными для кого-то
        if (group->primary_for_some_users) {
            pthread_mutex_unlock(&state_data_mutex);
            return -EIO;
        }

        g_hash_table_remove(state.groups, name);

        free(name);

        int save_res = save_groups_to_file();
        if (save_res != 0) {
            pthread_mutex_unlock(&state_data_mutex);
            return save_res;
        }

        deleted = true;
    }

    if (deleted) {
        get_users();
        get_groups();
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    pthread_mutex_unlock(&state_data_mutex);
    return -EINVAL;
}
