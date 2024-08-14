#define FUSE_USE_VERSION 31

#include <assert.h>
#include <fuse.h>
#include <glib.h>
#include <grp.h>
#include <limits.h> /* PATH_MAX */
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "./operations/operations.h"
#include "common.h"
#include "groups.h"
#include "users.h"

struct State state = { NULL, NULL };
char ABSOLUTE_MOUNT_PATH[PATH_MAX];
pthread_mutex_t state_data_mutex;

static struct options {
    int show_help;
} options;

static const struct fuse_opt option_spec[]
    = { OPTION("-h", show_help), OPTION("--help", show_help), FUSE_OPT_END };

static const struct fuse_operations umfs_oper = {
    .init = umfs_init,
    .getattr = umfs_getattr,
    .readdir = umfs_readdir,
    .open = umfs_open,
    .opendir = umfs_opendir,
    .read = umfs_read,
    .readlink = umfs_readlink,
    .mkdir = umfs_mkdir,
};

static void show_help(const char *progname)
{
    printf("usage: %s <mountpoint>\n\n", progname);
}

int main(int argc, char *argv[])
{
    pthread_mutex_init(&state_data_mutex, NULL);

    get_users();
    get_groups();

    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if (options.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }

    realpath(argv[1], ABSOLUTE_MOUNT_PATH);

    ret = fuse_main(args.argc, args.argv, &umfs_oper, NULL);
    fuse_opt_free_args(&args);
    pthread_mutex_destroy(&state_data_mutex);
    return ret;
}
