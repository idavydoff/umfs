#include <pwd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void free_user(struct User *user)
{
    if (user)
    {
        free(user->name);
        user->name = NULL;
        free(user->shell);
        user->shell = NULL;
        free(user->dir);
        user->dir = NULL;
        free(user);
        user = NULL;
    }
}

void get_users()
{
    if (state.users)
    {
        GList *keys = g_hash_table_get_keys(state.users);
        GList *keys_ptr = keys;

        while (keys_ptr)
        {
            User *user = g_hash_table_lookup(state.users, keys_ptr->data);
            free_user(user);
            free(keys_ptr->data);
            keys_ptr = keys_ptr->next;
        }

        g_list_free(keys);

        g_hash_table_steal_all(state.users);
    }
    else
    {
        state.users = g_hash_table_new(g_str_hash, g_str_equal);
    }

    struct passwd *p;
    while ((p = getpwent()) != NULL)
    {
        struct User *new_user = malloc(sizeof(struct User));

        new_user->name = strdup(p->pw_name);
        new_user->uid = p->pw_uid;
        new_user->dir = strdup(p->pw_dir);
        new_user->shell = strdup(p->pw_shell);

        g_hash_table_insert(state.users, strdup(p->pw_name), new_user);
    }
    endpwent();
}