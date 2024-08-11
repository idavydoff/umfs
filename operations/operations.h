#include <stddef.h>

#ifndef OPERATIONS /* Include guard */
#define OPERATIONS

void *umfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
int umfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int umfs_readlink(const char *path, char *buf, size_t buf_size);
int umfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int umfs_open(const char *path, struct fuse_file_info *fi);
int umfs_opendir(const char *path, struct fuse_file_info *fi);
int umfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);

#endif