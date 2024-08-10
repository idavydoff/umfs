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

struct State state = {NULL, NULL};
pthread_mutex_t state_data_mutex;

static struct options
{
    int show_help;
} options;

static const struct fuse_opt option_spec[] = {
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    FUSE_OPT_END};

static void *umfs_init(struct fuse_conn_info *conn,
                       struct fuse_config *cfg)
{
    (void)conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int umfs_getattr(const char *path, struct stat *stbuf,
                        struct fuse_file_info *fi)
{
    (void)fi;
    printf("Get attr: %s\n", path);

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
        return 0;
    }
    else if (strcmp(path, "/users") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        pthread_mutex_lock(&state_data_mutex);
        stbuf->st_nlink = 1 + g_hash_table_size(state.users);
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }
    else if (strcmp(path, "/groups") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        pthread_mutex_lock(&state_data_mutex);
        stbuf->st_nlink = 1 + g_hash_table_size(state.groups);
        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }
    else
    {
        if (startsWith(path, "/users/"))
        {
            pthread_mutex_lock(&state_data_mutex);
            char *name = get_item_name_from_path(path, "/users/");
            struct User *user = g_hash_table_lookup(state.users, name);

            if (user == NULL)
            {
                pthread_mutex_unlock(&state_data_mutex);
                return -ENOENT;
            }

            strcat(name, "/");

            if (startsWith(path + 7, name))
            {
                stbuf->st_mode = S_IFREG | 0666;
                stbuf->st_nlink = 1;

                char buffer[100];
                if (string_ends_with(path, "/uid") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%d\n", user->uid);
                    stbuf->st_size = get_dynamic_string_size(buffer);
                }
                if (string_ends_with(path, "/shell") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%s\n", user->shell);
                    stbuf->st_size = get_dynamic_string_size(buffer);
                }
                if (string_ends_with(path, "/dir") != 0)
                {
                    snprintf(buffer, sizeof(buffer), "%s\n", user->dir);
                    stbuf->st_size = get_dynamic_string_size(buffer);
                }

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }
            else
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
            }
        }

        if (startsWith(path, "/groups/"))
        {
            pthread_mutex_lock(&state_data_mutex);
            char *name = get_item_name_from_path(path, "/groups/");
            struct Group *group = g_hash_table_lookup(state.groups, name);

            if (group == NULL)
            {
                pthread_mutex_unlock(&state_data_mutex);
                return -ENOENT;
            }

            strcat(name, "/");

            char members_dir_name[58];
            strcpy(members_dir_name, group->name);
            strcat(members_dir_name, "/members");

            if (startsWith(path + 8, name))
            {
                if (strcmp(path + 8, members_dir_name) == 0)
                {
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
            else
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;

                pthread_mutex_unlock(&state_data_mutex);
                return 0;
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
    (void)offset;
    (void)fi;
    (void)flags;

    printf("Read dir: %s\n", path);

    pthread_mutex_lock(&state_data_mutex);
    get_groups();
    get_users();
    pthread_mutex_unlock(&state_data_mutex);

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    if (startsWith(path, "/users/"))
    {
        filler(buf, "uid", NULL, 0, 0);
        filler(buf, "shell", NULL, 0, 0);
        filler(buf, "dir", NULL, 0, 0);

        return 0;
    }
    pthread_mutex_lock(&state_data_mutex);
    if (strcmp(path, "/users") == 0)
    {
        GList *users_list = g_hash_table_get_keys(state.users);
        GList *users_list_ptr = users_list;

        while (users_list_ptr)
        {
            printf("%s\n", users_list_ptr->data);
            filler(buf, users_list_ptr->data, NULL, 0, 0);
            users_list_ptr = users_list_ptr->next;
        }

        g_list_free(users_list);

        pthread_mutex_unlock(&state_data_mutex);
        return 0;
    }

    if (startsWith(path, "/groups/"))
    {
        if (string_ends_with(path, "/members") != 0)
        {
            char *name = get_item_name_from_path(path, "/groups/");
            struct Group *group = g_hash_table_lookup(state.groups, name);

            if (group == NULL)
            {
                pthread_mutex_unlock(&state_data_mutex);

                return -ENOENT;
            }

            for (int k = 0; k < group->members_count; k++)
            {
                filler(buf, group->members[k], NULL, 0, 0);
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

    if (strcmp(path, "/groups") == 0)
    {
        GList *groups_list = g_hash_table_get_keys(state.groups);
        GList *groups_list_ptr = groups_list;

        while (groups_list_ptr)
        {
            filler(buf, groups_list_ptr->data, NULL, 0, 0);
            groups_list_ptr = groups_list_ptr->next;
        }

        g_list_free(groups_list);
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
    (void)fi;
    // if(strcmp(path+1, options.filename) != 0)
    //     return -ENOENT;

    pthread_mutex_lock(&state_data_mutex);
    if (startsWith(path, "/users/"))
    {
        char *name = get_item_name_from_path(path, "/users/");
        struct User *user = g_hash_table_lookup(state.users, name);

        if (user == NULL)
        {
            pthread_mutex_unlock(&state_data_mutex);
            return -ENOENT;
        }

        char buffer[200];
        if (string_ends_with(path, "dir") != 0)
        {
            snprintf(buffer, sizeof(buffer), "%s\n", user->dir);
        }
        if (string_ends_with(path, "shell") != 0)
        {
            snprintf(buffer, sizeof(buffer), "%s\n", user->shell);
        }
        if (string_ends_with(path, "uid") != 0)
        {
            snprintf(buffer, sizeof(buffer), "%d\n", user->uid);
        }
        len = strlen(buffer);
        if (offset < len)
        {
            if (offset + size > len)
                size = len - offset;
            memcpy(buf, buffer + offset, size);
        }
        else
            size = 0;
        pthread_mutex_unlock(&state_data_mutex);
        return size;
    }

    pthread_mutex_unlock(&state_data_mutex);
    return -ENOENT;
}

static const struct fuse_operations umfs_oper = {
    .init = umfs_init,
    .getattr = umfs_getattr,
    .readdir = umfs_readdir,
    .open = umfs_open,
    .read = umfs_read,
};

static void show_help(const char *progname)
{
    printf("usage: %s <mountpoint>\n\n", progname);
}

int main(int argc, char *argv[])
{
    pthread_mutex_init(&state_data_mutex, NULL);

    get_groups();
    get_users();

    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if (options.show_help)
    {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }

    ret = fuse_main(args.argc, args.argv, &umfs_oper, NULL);
    fuse_opt_free_args(&args);
    pthread_mutex_destroy(&state_data_mutex);
    return ret;
}
