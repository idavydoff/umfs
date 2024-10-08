#include "glib.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>

#include "../umfs.h"
#include "../users.h"
#include "../utils.h"

int umfs_write(const char *path, const char *data, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
    printf("Write: %s\n", path);

    char *file_name = get_path_end(path);
    char *user_name = get_item_name_from_path(path, "/users/");

    pthread_mutex_lock(&state_data_mutex);
    User *user = g_hash_table_lookup(state.users, user_name);

    if (strcmp(file_name, "directory") == 0) {
        user->dir = realloc(user->dir, (size + 1) * sizeof(char));
        memcpy(user->dir, data, size);
        user->dir[strcspn(user->dir, "\n")] = '\0';
    }
    if (strcmp(file_name, "shell") == 0) {
        user->shell = realloc(user->shell, (size + 1) * sizeof(char));
        memcpy(user->shell, data, size);
        user->shell[strcspn(user->shell, "\n")] = '\0';
    }
    if (strcmp(file_name, "primary_group") == 0) {
        char *tmp = malloc((size + 1) * sizeof(char));
        memcpy(tmp, data, size);
        tmp[strcspn(tmp, "\n")] = '\0';

        Group *group = g_hash_table_lookup(state.groups, tmp);
        free(tmp);

        if (group == NULL) {
            pthread_mutex_unlock(&state_data_mutex);
            free(file_name);
            return -EADDRNOTAVAIL;
        }

        user->gid = group->gid;

        // Нужно для того, чтобы после изменения первичной группы и выполнении
        // после этого `ls /users`, пользователь сразу видел правильные атрибуты
        // у папки /users/<name> (отражающие наличие группы sudo).
        //
        // Так как после записи выполняется `get_attr` для /users/<name>,
        // то если сразу после этого выполнить `ls /users`, `get_attr`
        // выполнится для всех папок в /users, кроме той, в которой мы только
        // что делали изменения. Соответственно пользователь для этой папки
        // увидит старое значение.
        char *old_primary_group_name = get_user_primary_group_name(user);
        if (strcmp(group->name, "sudo") == 0
            || (strcmp(group->name, "wheel")) == 0) {
            user->privileged = true;
        } else if (strcmp(old_primary_group_name, "sudo") == 0
            || strcmp(old_primary_group_name, "wheel") == 0) {
            user->privileged = false;
        }

        free(old_primary_group_name);
    }
    if (strcmp(file_name, "full_name") == 0) {
        user->gecos = realloc(user->gecos, (size + 1) * sizeof(char));
        memcpy(user->gecos, data, size);
        user->gecos[strcspn(user->gecos, "\n")] = '\0';
    }
    if ((string_ends_with(path, "/ssh_authorized_keys") != 0)
        && user->is_home_dir) {
        char home_ssh_dir[PATH_MAX];
        snprintf(home_ssh_dir, PATH_MAX, "%s/.ssh", user->dir);

        struct stat ssh_dir_stat;
        int stat_res = stat(home_ssh_dir, &ssh_dir_stat);

        if (stat_res != 0) {
            mkdir(home_ssh_dir, 0700);
        }

        char home_authorized_keys_file[PATH_MAX];
        snprintf(home_authorized_keys_file, PATH_MAX, "%s/.ssh/authorized_keys",
            user->dir);

        struct stat ssh_authorized_keys_file_stat;
        stat_res
            = stat(home_authorized_keys_file, &ssh_authorized_keys_file_stat);

        FILE *fp = fopen(home_authorized_keys_file, "w");
        if (fp == NULL) {
            pthread_mutex_unlock(&state_data_mutex);
            free(file_name);
            return -EIO;
        }

        fwrite(data, 1, size, fp);
        fclose(fp);

        pthread_mutex_unlock(&state_data_mutex);
        free(file_name);
        return size;
    }

    int save_res = save_users_to_file();
    if (save_res != 0) {
        pthread_mutex_unlock(&state_data_mutex);
        return save_res;
    }

    pthread_mutex_unlock(&state_data_mutex);

    free(file_name);

    return size;
}