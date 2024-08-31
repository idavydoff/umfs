#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../groups.h"
#include "../umfs.h"
#include "../users.h"
#include "../utils.h"

uid_t create_group(char *name)
{
    if (g_hash_table_contains(state.groups, name)) {
        return 0;
    }

    uid_t gid = get_avalable_gid();

    FILE *fp = fopen("/etc/group", "a");
    if (fp == NULL) {
        return 1;
    }
    fprintf(fp, "%s:x:%d:\n", name, gid);
    fclose(fp);

    return gid;
}

int umfs_mkdir(const char *path, mode_t mode)
{
    printf("mkdir: %s\n", path);

    bool created = false;
    char *path_dup = strdup(path);

    if (startsWith(path, "/users/")) {
        if (strchr(path + strlen("/users/"), '/')) {
            free(path_dup);
            return -EADDRNOTAVAIL;
        }
        char *name = path_dup + strlen("/users/");

        pthread_mutex_lock(&state_data_mutex);
        if (g_hash_table_contains(state.users, name)) {
            free(path_dup);
            pthread_mutex_unlock(&state_data_mutex);
            return -EADDRINUSE;
        }
        pthread_mutex_unlock(&state_data_mutex);

        pthread_mutex_lock(&state_data_mutex);
        uid_t uid = get_avalable_uid();
        uid_t gid = create_group(name);

        short int group_suffix = 1;
        while (gid == 0) {
            char tmp[100];
            sprintf(tmp, "%s%d", name, group_suffix);
            gid = create_group(tmp);
            if (gid == 1) {
                pthread_mutex_unlock(&state_data_mutex);
                return -EIO;
            }
            group_suffix++;
        }
        pthread_mutex_unlock(&state_data_mutex);

        FILE *fp = fopen("/etc/passwd", "a");
        if (fp == NULL) {
            free(path_dup);
            return -EIO;
        }
        fprintf(fp, "%s:x:%d:%d::/home/%s:/bin/bash\n", name, uid, gid, name);
        fclose(fp);

        struct stat st;

        char home_path[PATH_MAX];
        sprintf(home_path, "/home/%s", name);

        if (stat(home_path, &st) == -1) {
            mkdir(home_path, 0700);
        }

        created = true;
    }

    if (startsWith(path, "/groups/")) {
        if (strchr(path + strlen("/groups/"), '/')) {
            free(path_dup);
            return -EADDRNOTAVAIL;
        }
        char *name = path_dup + strlen("/groups/");

        pthread_mutex_lock(&state_data_mutex);
        uid_t gid = create_group(name);
        pthread_mutex_unlock(&state_data_mutex);

        if (gid == 0) {
            free(path_dup);
            return -EADDRINUSE;
        } else if (gid == 1) {
            free(path_dup);
            return -EIO;
        }

        created = true;
    }

    free(path_dup);

    if (created) {
        pthread_mutex_lock(&state_data_mutex);
        get_users();
        get_groups();
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    return -EADDRNOTAVAIL;
}