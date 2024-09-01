#include <string.h>
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <glib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../groups.h"
#include "../umfs.h"
#include "../users.h"
#include "../utils.h"

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

        char buffer[10000];
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
            snprintf(buffer, sizeof(buffer), "%s\n", primary_group_name);
            free(primary_group_name);
        }
        if (string_ends_with(path, "/full_name") != 0) {
            snprintf(buffer, sizeof(buffer), "%s\n", user->gecos);
        }
        if ((string_ends_with(path, "/ssh_authorized_keys") != 0)
            && user->is_home_dir) {
            char home_ssh_dir[PATH_MAX];
            snprintf(home_ssh_dir, PATH_MAX, "%s/.ssh", user->dir);

            struct stat ssh_dir_stat;
            int stat_res = stat(home_ssh_dir, &ssh_dir_stat);

            if (stat_res == 0) {
                char home_authorized_keys_file[PATH_MAX];
                snprintf(home_authorized_keys_file, PATH_MAX,
                    "%s/.ssh/authorized_keys", user->dir);

                struct stat ssh_authorized_keys_file_stat;
                stat_res = stat(
                    home_authorized_keys_file, &ssh_authorized_keys_file_stat);

                if (stat_res == 0) {
                    FILE *fp = fopen(home_authorized_keys_file, "r");
                    if (fp == NULL) {
                        pthread_mutex_unlock(&state_data_mutex);
                        return 0;
                    }

                    fread(buffer, 1, ssh_authorized_keys_file_stat.st_size, fp);
                    fclose(fp);
                }
            }
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
        if (string_ends_with(path, "id") != 0) {
            snprintf(buffer, sizeof(buffer), "%d\n", group->gid);
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