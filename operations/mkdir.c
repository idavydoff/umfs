#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h> /* PATH_MAX */

#include "../common.h"
#include "../users.h"
#include "../groups.h"

int umfs_mkdir(const char *path, mode_t mode)
{
    printf("Creating dir: %s\n", path);

    bool created = false;

    char *str_dup = strdup(path);
    if (startsWith(path, "/users/"))
    {
        if (strchr(path + strlen("/users/"), '/'))
        {
            return -EADDRNOTAVAIL;
        }
        char *name = str_dup + strlen("/users/");
        printf("Creating user: %s\n", name);

        if (g_hash_table_contains(state.users, name))
        {
            return -EADDRINUSE;
        }

        uid_t uid = get_avalable_uid();

        FILE *fp = fopen("/etc/passwd", "a");
        fprintf(fp, "%s:x:%d:%d::/home/%s:/bin/bash\n", name, uid, uid, name);
        fclose(fp);

        struct stat st = {0};

        char home_path[PATH_MAX];
        sprintf(home_path, "/home/%s", name);

        if (stat(home_path, &st) == -1)
        {
            mkdir(home_path, 0700);
        }

        created = true;
    }

    if (startsWith(path, "/groups/"))
    {
        if (strchr(path + strlen("/groups/"), '/'))
        {
            return -EADDRNOTAVAIL;
        }
        char *name = str_dup + strlen("/groups/");
        printf("Creating group: %s\n", name);

        if (g_hash_table_contains(state.groups, name))
        {
            return -EADDRINUSE;
        }

        uid_t gid = get_avalable_gid();

        FILE *fp = fopen("/etc/group", "a");
        fprintf(fp, "%s:x:%d:\n", name, gid);
        fclose(fp);

        created = true;
    }

    free(str_dup);

    if (created)
    {
        pthread_mutex_lock(&state_data_mutex);
        get_users();
        get_groups();
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    return -EADDRNOTAVAIL;
}