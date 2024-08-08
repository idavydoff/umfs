#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <glib.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>

#include <grp.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <pthread.h>

#include "common.h"
#include "users.h"
#include "groups.h"

struct State state = {NULL, 0, NULL, 0};
pthread_mutex_t state_data_mutex;

static struct options {
    int show_help;
} options;

static const struct fuse_opt option_spec[] = {
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    FUSE_OPT_END
};

static void *umfs_init(struct fuse_conn_info *conn,
            struct fuse_config *cfg)
{
    (void) conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int umfs_getattr(const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
    (void) fi;
    printf("Get attr: %s\n", path);

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
        return 0;
    } else if (strcmp(path, "/users") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 1 + state.users_count;
        return 0;
    } else if (strcmp(path, "/groups") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 1 + state.groups_count;
        return 0;
    } else {
        if (startsWith(path, "/users/")) {
            pthread_mutex_lock(&state_data_mutex);
            for(int i = 0; i < state.users_count; i++)
            {
                char name[50];
                strcpy(name, state.users[i]->name);
                strcat(name, "/");

                if (strcmp(path+7, state.users[i]->name) == 0) {
                    stbuf->st_mode = S_IFDIR | 0755;
                    stbuf->st_nlink = 2;

                    pthread_mutex_unlock(&state_data_mutex);
                    return 0;
                } else if (startsWith(path+7, name)) {
                    stbuf->st_mode = S_IFREG | 0666;
                    stbuf->st_nlink = 1;

                    char buffer[100];
                    if (string_ends_with(path, "/uid") != 0) {
                        snprintf(buffer, sizeof(buffer), "%d\n", state.users[i]->uid);
                        stbuf->st_size = get_dynamic_string_size(buffer);
                    }
                    if (string_ends_with(path, "/shell") != 0) {
                        snprintf(buffer, sizeof(buffer), "%s\n", state.users[i]->shell);
                        stbuf->st_size = get_dynamic_string_size(buffer);
                    }
                    if (string_ends_with(path, "/dir") != 0) {
                        snprintf(buffer, sizeof(buffer), "%s\n", state.users[i]->dir);
                        stbuf->st_size = get_dynamic_string_size(buffer);
                    }

                    pthread_mutex_unlock(&state_data_mutex);
                    return 0;
                }
            }
        }

        if (startsWith(path, "/groups/")) {
            pthread_mutex_lock(&state_data_mutex);
            for(int i = 0; i < state.groups_count; i++)
            {
                printf("Getting index: %d, count: %d\n", i, state.groups_count);

                char name[50];
                strcpy(name, state.groups[i]->name);
                strcat(name, "/");

                char members_dir_name[58];
                strcpy(members_dir_name, state.groups[i]->name);
                strcat(members_dir_name, "/members");

                if (strcmp(path+8, state.groups[i]->name) == 0) {
                    stbuf->st_mode = S_IFDIR | 0755;
                    stbuf->st_nlink = 2;

                    pthread_mutex_unlock(&state_data_mutex);
                    return 0;
                } else if (startsWith(path+8, name)) {
                    if (strcmp(path+8, members_dir_name) == 0) {
                        stbuf->st_mode = S_IFDIR | 0755;
                        stbuf->st_nlink = 2;

                        pthread_mutex_unlock(&state_data_mutex);
                        return 0;
                    }
                    stbuf->st_mode = S_IFREG | 0666;
                    stbuf->st_nlink = 1;
                    stbuf->st_size = 100; // TODO убрать

                    pthread_mutex_unlock(&state_data_mutex);
                    return 0;
                }
            }
        }
    }

    pthread_mutex_unlock(&state_data_mutex);
    return -ENOENT;
}

static int umfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi,
             enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    printf("Read dir: %s\n", path);

    pthread_mutex_lock(&state_data_mutex);
    get_groups();
    get_users();
    pthread_mutex_unlock(&state_data_mutex);

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    if (startsWith(path, "/users/")) {
        filler(buf, "uid", NULL, 0, 0);
        filler(buf, "shell", NULL, 0, 0);
        filler(buf, "dir", NULL, 0, 0);

        return 0;
    }
    pthread_mutex_lock(&state_data_mutex);
    if (strcmp(path, "/users") == 0) {
        for(int i = 0; i < state.users_count; i++)
        {
            filler(buf, state.users[i]->name, NULL, 0, 0);
        }

        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    if (startsWith(path, "/groups/")) {
        if (string_ends_with(path, "/members") != 0) {
            for (int i = 0; i < state.groups_count; i++) {
                char groupPath[80] = "/groups/";
                strcat(groupPath, state.groups[i]->name);
                strcat(groupPath, "/members");


                if (strcmp(groupPath, path) == 0) {
                    for(int k = 0; k < state.groups[i]->members_count; k++)
                    {
                        filler(buf, state.groups[i]->members[k], NULL, 0, 0);
                    }

                    pthread_mutex_unlock(&state_data_mutex);
                    return 0;
                }
            }
            
            pthread_mutex_unlock(&state_data_mutex);
            return 0;
        }
		
        filler(buf, "gid", NULL, 0, 0);
        filler(buf, "name", NULL, 0, 0);
        filler(buf, "password", NULL, 0, 0);
        filler(buf, "members", NULL, 0, 0);
        
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    if (strcmp(path, "/groups") == 0) {
        for(int i = 0; i < state.users_count; i++)
        {
            filler(buf, state.groups[i]->name, NULL, 0, 0);
        }
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }



    filler(buf, "users", NULL, 0, 0);
    filler(buf, "groups", NULL, 0, 0);

    pthread_mutex_unlock(&state_data_mutex);
    return 0;
}

static int umfs_open(const char *path, struct fuse_file_info *fi)
{
    printf("Open: %s\n", path);
    // if (strcmp(path+1, options.filename) != 0)
    //  return -ENOENT;
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    fi->keep_cache = 1;

    return 0;
}

static int umfs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    printf("Read: %s\n", path);
    size_t len;
    (void) fi;
    // if(strcmp(path+1, options.filename) != 0)
    //     return -ENOENT;

    pthread_mutex_lock(&state_data_mutex);
    for(int i = 0; i < state.users_count; i++)
    {
        if (startsWith(path+7, state.users[i]->name)) {
            char buffer[100];
            if (string_ends_with(path, "dir") != 0) {
                snprintf(buffer, sizeof(buffer), "%s\n", state.users[i]->dir);
            }
            if (string_ends_with(path, "shell") != 0) {
                snprintf(buffer, sizeof(buffer), "%s\n", state.users[i]->shell);
            }
            if (string_ends_with(path, "uid") != 0) {
                snprintf(buffer, sizeof(buffer), "%d\n", state.users[i]->uid);
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
    }


    pthread_mutex_unlock(&state_data_mutex);
    return -ENOENT;
}

static const struct fuse_operations umfs_oper = {
    .init       = umfs_init,
    .getattr    = umfs_getattr,
    .readdir    = umfs_readdir,
    .open       = umfs_open,
    .read       = umfs_read,
};


static void show_help(const char *progname)
{
    printf("usage: %s <mountpoint>\n\n", progname);
}

int main(int argc, char *argv[])
{
    pthread_mutex_init(&state_data_mutex, NULL);

    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if (options.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }

	// HashTable Example
	// GHashTable *hashTable = g_hash_table_new(g_str_hash, g_str_equal);

	// g_hash_table_insert(hashTable, "Jazzy", "Cheese");
	// g_hash_table_insert(hashTable, "Mr Darcy", "Treats");

	// printf("There are %d keys in the hash table\n", g_hash_table_size(hashTable));

	// printf("Jazzy likes %s\n", g_hash_table_lookup(hashTable, "Jazzy"));

	// g_hash_table_destroy(hashTable);


	ret = fuse_main(args.argc, args.argv, &umfs_oper, NULL);
	fuse_opt_free_args(&args);
    pthread_mutex_destroy(&state_data_mutex);
	return ret;
}
