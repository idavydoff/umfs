#define FUSE_USE_VERSION 31

#include <fuse.h>

#include "../umfs.h"

void *umfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    cfg->kernel_cache = 1;
    return NULL;
}