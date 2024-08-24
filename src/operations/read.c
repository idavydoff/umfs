#include <string.h>
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <glib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common.h"
#include "../groups.h"
#include "../users.h"

int umfs_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
    printf("Read: %s\n", path);
    size_t len;
    (void)fi;
    // if(strcmp(path+1, options.filename) != 0)
    //     return -ENOENT;

    pthread_mutex_lock(&state_data_mutex);
    if (startsWith(path, "/users/")) {
        char *name = get_item_name_from_path(path, "/users/");
        struct User *user = g_hash_table_lookup(state.users, name);

        if (user == NULL) {
            pthread_mutex_unlock(&state_data_mutex);
            return -ENOENT;
        }

        char buffer[200];
        if (string_ends_with(path, "/directory") != 0) {
            snprintf(buffer, sizeof(buffer), "%s\n", user->dir);
        }
        if (string_ends_with(path, "/shell") != 0) {
            snprintf(buffer, sizeof(buffer), "%s\n", user->shell);
        }
        if (string_ends_with(path, "/id") != 0) {
            snprintf(buffer, sizeof(buffer), "%d\n", user->uid);
        }
        if (string_ends_with(path, "/primary_group") != 0) {
            char *primary_group_name = get_user_primary_group_name(user);
            snprintf(buffer, sizeof(buffer), "%d (%s)\n", user->gid,
                primary_group_name);
            free(primary_group_name);
        }
        if (string_ends_with(path, "/full_name") != 0) {
            snprintf(buffer, sizeof(buffer), "%s\n", user->gecos);
        }
        if (string_ends_with(path, "/name") != 0) {
            snprintf(buffer, sizeof(buffer), "%s\n", user->name);
        }
        len = strlen(buffer);
        if (offset < len) {
            if (offset + size > len)
                size = len - offset;
            memcpy(buf, buffer + offset, size);
        } else
            size = 0;
        pthread_mutex_unlock(&state_data_mutex);
        return size;
    }

    if (startsWith(path, "/groups/")) {
        char *name = get_item_name_from_path(path, "/groups/");
        struct Group *group = g_hash_table_lookup(state.groups, name);

        if (group == NULL) {
            pthread_mutex_unlock(&state_data_mutex);
            return -ENOENT;
        }

        char buffer[200];
        if (string_ends_with(path, "gid") != 0) {
            snprintf(buffer, sizeof(buffer), "%d\n", group->gid);
        }
        if (string_ends_with(path, "name") != 0) {
            snprintf(buffer, sizeof(buffer), "%s\n", group->name);
        }
        len = strlen(buffer);
        if (offset < len) {
            if (offset + size > len)
                size = len - offset;
            memcpy(buf, buffer + offset, size);
        } else
            size = 0;
        pthread_mutex_unlock(&state_data_mutex);
        return size;
    }

    pthread_mutex_unlock(&state_data_mutex);
    return -ENOENT;
}