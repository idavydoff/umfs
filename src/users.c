#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "common.h"

void free_user(void *user)
{
    User *user_ptr = user;
    if (user_ptr) {
        free(user_ptr->name);
        user_ptr->name = NULL;
        free(user_ptr->gecos);
        user_ptr->gecos = NULL;
        free(user_ptr->shell);
        user_ptr->shell = NULL;
        free(user_ptr->dir);
        user_ptr->dir = NULL;

        for (int i = 0; i < user_ptr->groups_count; i++) {
            free(user_ptr->groups[i]);
            user_ptr->groups[i] = NULL;
        }
        free(user_ptr->groups);
        user_ptr->groups = NULL;

        free(user_ptr);
        user_ptr = NULL;
    }
}

User *copy_user(User *source_user)
{
    static User *new_user;
    new_user = malloc(sizeof(struct User));

    new_user->name = strdup(source_user->name);
    new_user->gecos = strdup(source_user->gecos);
    new_user->uid = source_user->uid;
    new_user->gid = source_user->gid;
    new_user->dir = strdup(source_user->dir);
    new_user->shell = strdup(source_user->shell);

    new_user->groups = malloc(source_user->groups_count * sizeof(char *));
    for (int i = 0; i < source_user->groups_count; i++) {
        new_user->groups[i] = strdup(source_user->groups[i]);
    }
    new_user->groups_count = source_user->groups_count;
    new_user->sudo = source_user->sudo;

    return new_user;
}

void get_users()
{
    if (state.users) {
        g_hash_table_remove_all(state.users);
    } else {
        state.users
            = g_hash_table_new_full(g_str_hash, g_str_equal, &free, &free_user);
    }

    struct passwd *p;
    while ((p = getpwent()) != NULL) {
        struct User *new_user = malloc(sizeof(struct User));

        new_user->name = strdup(p->pw_name);
        new_user->gecos = strdup(p->pw_gecos);
        new_user->uid = p->pw_uid;
        new_user->gid = p->pw_gid;
        new_user->dir = strdup(p->pw_dir);
        new_user->shell = strdup(p->pw_shell);
        new_user->sudo = false;

        new_user->groups = malloc(5 * sizeof(char *));
        new_user->groups_count = 0;

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

uid_t get_avalable_uid()
{
    return get_avalable_id(&get_users_keys, &get_user_uid);
}

char *get_user_primary_group_name(User *user)
{
    static char *primary_group_name = NULL;

    GList *groups_values = g_hash_table_get_values(state.groups);
    GList *groups_values_ptr = groups_values;

    while (groups_values_ptr) {
        Group *group = groups_values_ptr->data;

        if (group->gid == user->gid) {
            primary_group_name = strdup(group->name);
            break;
        }

        groups_values_ptr = groups_values_ptr->next;
    }

    g_list_free(groups_values);

    return primary_group_name;
}