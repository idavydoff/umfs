#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>

#include "../groups.h"
#include "../umfs.h"
#include "../users.h"

int umfs_opendir(const char *path, struct fuse_file_info *fi)
{
    printf("Open dir: %s\n", path);
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    pthread_mutex_lock(&state_data_mutex);
    get_users();
    get_groups();
    pthread_mutex_unlock(&state_data_mutex);

    fi->keep_cache = 1;

    return 0;
}
