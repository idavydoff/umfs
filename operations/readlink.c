#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>

#include "../common.h"

int umfs_readlink(const char *path, char *buf, size_t buf_size)
{
    printf("Read link: %s\n", path);
    char newpath[buf_size];

    if (startsWith(path, "/users/")) {
        char *group_name = get_path_end(path);

        // TODO: урезать новый путь, если он не влезает в newpath[buf_size].
        // (Как урезать чтобы симлинк продолжал резолвиться в правильный путь? Хуй его
        // знает)
        sprintf(newpath, "%s/groups/%s\0", ABSOLUTE_MOUNT_PATH, group_name);
        free(group_name);
        memcpy(buf, newpath, strlen(newpath) + 1);
    }
    if (startsWith(path, "/groups/")) {
        char *member_name = get_path_end(path);

        // TODO: урезать новый путь, если он не влезает в newpath[buf_size].
        // (Как урезать чтобы симлинк продолжал резолвиться в правильный путь? Хуй его
        // знает)
        sprintf(newpath, "%s/users/%s\0", ABSOLUTE_MOUNT_PATH, member_name);
        free(member_name);
        memcpy(buf, newpath, strlen(newpath) + 1);
    }

    return 0;
}