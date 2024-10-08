#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <glib.h>
#include <string.h>

#include <stdio.h>

#include "../groups.h"
#include "../umfs.h"
#include "../users.h"
#include "../utils.h"

int umfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    printf("Read dir: %s\n", path);

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    if (startsWith(path, "/users/")) {
        if (string_ends_with(path, "/groups") != 0) {
            pthread_mutex_lock(&state_data_mutex);
            char *name = get_item_name_from_path(path, "/users/");
            struct User *user = g_hash_table_lookup(state.users, name);

            if (user == NULL) {
                pthread_mutex_unlock(&state_data_mutex);

                return -ENOENT;
            }

            for (int k = 0; k < user->groups_count; k++) {
                filler(buf, user->groups[k], NULL, 0, 0);
            }

            pthread_mutex_unlock(&state_data_mutex);
            return 0;
        }

        filler(buf, "id", NULL, 0, 0);
        filler(buf, "primary_group", NULL, 0, 0);
        filler(buf, "shell", NULL, 0, 0);
        filler(buf, "directory", NULL, 0, 0);
        filler(buf, "groups", NULL, 0, 0);
        filler(buf, "full_name", NULL, 0, 0);
        filler(buf, "ssh_authorized_keys", NULL, 0, 0);

        return 0;
    }

    if (strcmp(path, "/users") == 0) {
        pthread_mutex_lock(&state_data_mutex);
        GList *users_list = g_hash_table_get_keys(state.users);
        GList *users_list_ptr = users_list;

        while (users_list_ptr) {
            filler(buf, users_list_ptr->data, NULL, 0, 0);
            users_list_ptr = users_list_ptr->next;
        }

        g_list_free(users_list);

        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    if (startsWith(path, "/groups/")) {
        if (string_ends_with(path, "/members") != 0) {
            pthread_mutex_lock(&state_data_mutex);
            char *name = get_item_name_from_path(path, "/groups/");
            struct Group *group = g_hash_table_lookup(state.groups, name);

            if (group == NULL) {
                pthread_mutex_unlock(&state_data_mutex);

                return -ENOENT;
            }

            for (int k = 0; k < group->members_count; k++) {
                filler(buf, group->members[k], NULL, 0, 0);
            }

            pthread_mutex_unlock(&state_data_mutex);
            return 0;
        }

        filler(buf, "id", NULL, 0, 0);
        filler(buf, "members", NULL, 0, 0);

        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    if (strcmp(path, "/groups") == 0) {
        pthread_mutex_lock(&state_data_mutex);
        GList *groups_list = g_hash_table_get_keys(state.groups);
        GList *groups_list_ptr = groups_list;

        while (groups_list_ptr) {
            filler(buf, groups_list_ptr->data, NULL, 0, 0);
            groups_list_ptr = groups_list_ptr->next;
        }

        g_list_free(groups_list);
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    filler(buf, "users", NULL, 0, 0);
    filler(buf, "groups", NULL, 0, 0);

    return 0;
}
