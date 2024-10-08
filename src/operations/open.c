#include <fcntl.h>
#include <pthread.h>
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>

#include "../umfs.h"
#include "../utils.h"

int umfs_open(const char *path, struct fuse_file_info *fi)
{
    printf("Open: %s\n", path);

    pthread_mutex_lock(&state_data_mutex);
    if (state.fake_file_path != NULL) {
        if (strcmp(path, state.fake_file_path) == 0) {
            free(state.fake_file_path);
            state.fake_file_path = NULL;
            pthread_mutex_unlock(&state_data_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&state_data_mutex);

    if ((fi->flags & O_ACCMODE) != O_RDONLY && startsWith(path, "/users/")) {
        char *name = get_item_name_from_path(path, "/users/");
        pthread_mutex_lock(&state_data_mutex);
        struct User *user = g_hash_table_lookup(state.users, name);
        pthread_mutex_unlock(&state_data_mutex);

        if (user == NULL) {
            return -ENOENT;
        }

        bool can_write = string_ends_with(path, "/directory") != 0
            || string_ends_with(path, "/shell") != 0
            || string_ends_with(path, "/primary_group") != 0
            || string_ends_with(path, "/full_name") != 0
            || (user->is_home_dir
                    ? string_ends_with(path, "/ssh_authorized_keys") != 0
                    : false);

        if (can_write)
            return 0;
    }

    fi->keep_cache = 1;

    if ((fi->flags & O_ACCMODE) == O_RDONLY) {
        return 0;
    }

    return -EACCES;
}