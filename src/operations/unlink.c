#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>

#include "../groups.h"
#include "../umfs.h"
#include "../users.h"
#include "../utils.h"

int umfs_unlink(const char *path)
{
    printf("Unlink: %s\n", path);

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

            short deleted_count = delete_from_dynamic_string_array(
                &group->members, group->members_count, user_name);

            free(user_name);

            group->members_count -= deleted_count;

            save_groups_to_file();
            pthread_mutex_unlock(&state_data_mutex);
            return 0;
        }
        return -EADDRNOTAVAIL;
    }

    return -EADDRNOTAVAIL;
}
