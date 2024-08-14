#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "common.h"

void free_user(struct User *user)
{
    if (user) {
        free(user->name);
        user->name = NULL;
        free(user->gecos);
        user->gecos = NULL;
        free(user->shell);
        user->shell = NULL;
        free(user->dir);
        user->dir = NULL;

        for (int i = 0; i < user->groups_count; i++) {
            free(user->groups[i]);
            user->groups[i] = NULL;
        }
        free(user->groups);
        user->groups = NULL;

        free(user);
        user = NULL;
    }
}

void get_users()
{
    if (state.users) {
        GList *keys = g_hash_table_get_keys(state.users);
        GList *keys_ptr = keys;

        while (keys_ptr) {
            User *user = g_hash_table_lookup(state.users, keys_ptr->data);
            free_user(user);
            free(keys_ptr->data);
            keys_ptr = keys_ptr->next;
        }

        g_list_free(keys);

        g_hash_table_steal_all(state.users);
    } else {
        state.users = g_hash_table_new(g_str_hash, g_str_equal);
    }

    struct passwd *p;
    while ((p = getpwent()) != NULL) {
        struct User *new_user = malloc(sizeof(struct User));

        new_user->name = strdup(p->pw_name);
        new_user->gecos = strdup(p->pw_gecos);
        new_user->uid = p->pw_uid;
        new_user->dir = strdup(p->pw_dir);
        new_user->shell = strdup(p->pw_shell);

        new_user->groups = malloc(5 * sizeof(char *));
        new_user->groups[0] = strdup(p->pw_name);
        new_user->groups_count = 1;

        g_hash_table_insert(state.users, strdup(p->pw_name), new_user);
    }
    endpwent();
}

GList *get_users_keys() { return g_hash_table_get_keys(state.users); }

uid_t get_user_uid(char *name)
{
    User *user = g_hash_table_lookup(state.users, name);

    return user->uid;
}

uid_t get_avalable_uid() { return get_avalable_id(&get_users_keys, &get_user_uid); }