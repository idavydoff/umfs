#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>

#include "../umfs.h"
#include "../utils.h"

int umfs_mknod(const char *path, mode_t mode, dev_t dev)
{
    printf("Mknod: %s\n", path);

    if (startsWith(path, "/groups/")) {
        pthread_mutex_lock(&state_data_mutex);
        char *name = get_item_name_from_path(path, "/groups/");
        struct Group *group = g_hash_table_lookup(state.groups, name);

        if (group == NULL) {
            pthread_mutex_unlock(&state_data_mutex);
            return -ENOENT;
        }

        char members_dir_name[NAME_MAX + strlen("/members")];
        strcpy(members_dir_name, group->name);
        strcat(members_dir_name, "/members");

        // /groups/<name>/members/<member_name>
        if (!string_ends_with(path, members_dir_name)
            && startsWith(path + strlen("/groups/"), members_dir_name)) {
            char *user_name = get_path_end(path);

            User *user = g_hash_table_lookup(state.users, user_name);
            free(user_name);

            if (user == NULL) {
                pthread_mutex_unlock(&state_data_mutex);
                return -EADDRNOTAVAIL;
            }

            if (group->members_count % 10 > 0) {
                group->members[group->members_count] = strdup(user->name);
                group->members_count++;
            } else {
                group->members = realloc(group->members,
                    (group->members_count + 1) * sizeof(char *));
                group->members[group->members_count] = strdup(user->name);
                group->members_count++;
            }

            state.fake_file_path = strdup(path);

            save_groups_to_file();
            pthread_mutex_unlock(&state_data_mutex);
            return 0;
        }
    }

    return -EADDRNOTAVAIL;
}