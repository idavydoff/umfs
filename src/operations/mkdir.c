#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common.h"
#include "../groups.h"
#include "../users.h"

uid_t create_group(char *name)
{
    pthread_mutex_lock(&state_data_mutex);
    if (g_hash_table_contains(state.groups, name)) {
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    uid_t gid = get_avalable_gid();

    FILE *fp = fopen("/etc/group", "a");
    fprintf(fp, "%s:x:%d:\n", name, gid);
    fclose(fp);

    pthread_mutex_unlock(&state_data_mutex);

    return gid;
}

int umfs_mkdir(const char *path, mode_t mode)
{
    printf("mkdir: %s\n", path);

    bool created = false;
    char *path_dup = strdup(path);

    if (startsWith(path, "/users/")) {
        if (strchr(path + strlen("/users/"), '/')) {
            return -EADDRNOTAVAIL;
        }
        char *name = path_dup + strlen("/users/");

        pthread_mutex_lock(&state_data_mutex);
        if (g_hash_table_contains(state.users, name)) {
            pthread_mutex_unlock(&state_data_mutex);
            return -EADDRINUSE;
        }
        pthread_mutex_unlock(&state_data_mutex);

        uid_t uid = get_avalable_uid();
        uid_t gid = create_group(name);

        short int group_suffix = 1;
        while (gid == 0) {
            char tmp[100];
            sprintf(tmp, "%s%d", name, group_suffix);
            gid = create_group(tmp);
            group_suffix++;
        }

        FILE *fp = fopen("/etc/passwd", "a");
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
            return -EADDRNOTAVAIL;
        }
        char *name = path_dup + strlen("/groups/");

        uid_t gid = create_group(name);

        if (gid == 0) {
            return -EADDRINUSE;
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