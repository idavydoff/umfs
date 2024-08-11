#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>

#include "../common.h"

int umfs_opendir(const char *path, struct fuse_file_info *fi)
{
    printf("Open dir: %s\n", path);
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    fi->keep_cache = 1;

    return 0;
}
