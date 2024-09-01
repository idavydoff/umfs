#include <glib.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "umfs.h"
#include "utils.h"

void free_group(void *ptr)
{
    Group *group = ptr;

    if (group) {
        free(group->name);
        group->name = NULL;

        for (int i = 0; i < group->members_count; i++) {
            free(group->members[i]);
            group->members[i] = NULL;
        }
        free(group->members);
        group->members = NULL;

        free(group);
        group = NULL;
    }
}

Group *copy_group(Group *source_group)
{
    static Group *new_group;
    new_group = malloc(sizeof(struct Group));

    new_group->name = strdup(source_group->name);
    new_group->gid = source_group->gid;

    new_group->members = malloc(source_group->members_count * sizeof(char *));
    for (int i = 0; i < source_group->members_count; i++) {
        new_group->members[i] = strdup(source_group->members[i]);
    }
    new_group->members_count = source_group->members_count;
    new_group->primary_for_some_users = source_group->primary_for_some_users;

    return new_group;
}

void get_groups()
{
    if (state.groups) {
        g_hash_table_remove_all(state.groups);
    } else {
        state.groups = g_hash_table_new_full(
            g_str_hash, g_str_equal, &free, &free_group);
    }

    struct group *grp;
    while ((grp = getgrent()) != NULL) {

        struct Group *new_group = malloc(sizeof(struct Group));

        new_group->gid = grp->gr_gid;
        new_group->name = strdup(grp->gr_name);

        int members_capacity = 10;
        new_group->members = malloc(members_capacity * sizeof(char *));
        new_group->members_count = 0;

        GList *users_keys = g_hash_table_get_keys(state.users);
        GList *users_keys_ptr = users_keys;

        // Находим всех юзеров, у которых gid совпадает с gid'ом текущей группы,
        // и добавляем их в список member'ов группы и добавляем саму группу
        // в список групп пользователя.
        while (users_keys_ptr) {
            User *user = g_hash_table_lookup(state.users, users_keys_ptr->data);
            if (user == NULL) {
                users_keys_ptr = users_keys_ptr->next;
                continue;
            }

            if (user->gid == grp->gr_gid) {
                if (new_group->members_count >= members_capacity) {
                    members_capacity += 10;
                    new_group->members = realloc(
                        new_group->members, members_capacity * sizeof(char *));
                }

                new_group->members[new_group->members_count]
                    = strdup(users_keys_ptr->data);
                new_group->members_count++;
                user->groups[user->groups_count] = strdup(new_group->name);
                user->groups_count++;

                if (strcmp(new_group->name, "sudo") == 0
                    || strcmp(new_group->name, "wheel") == 0) {
                    user->privileged = true;
                }

                new_group->primary_for_some_users = true;
            }
            users_keys_ptr = users_keys_ptr->next;
        }

        g_list_free(users_keys);

        while (*grp->gr_mem) {
            if (new_group->members_count >= members_capacity) {
                members_capacity += 10;
                new_group->members = realloc(
                    new_group->members, members_capacity * sizeof(char *));
            }

            new_group->members[new_group->members_count] = strdup(*grp->gr_mem);

            User *user = g_hash_table_lookup(state.users, *grp->gr_mem);
            if (user == NULL) {
                grp->gr_mem++;
                new_group->members_count++;
                continue;
            }
            if ((user->groups_count + 1) % 5 == 1) {
                user->groups = realloc(
                    user->groups, (user->groups_count + 5) * sizeof(char *));
            }
            user->groups[user->groups_count] = strdup(grp->gr_name);
            user->groups_count++;

            if (strcmp(new_group->name, "sudo") == 0
                || strcmp(new_group->name, "wheel") == 0) {
                user->privileged = true;
            }

            grp->gr_mem++;
            new_group->members_count++;
        }

        g_hash_table_insert(state.groups, strdup(grp->gr_name), new_group);
    }
    endgrent();
}

GList *get_groups_keys() { return g_hash_table_get_keys(state.groups); }

uid_t get_group_uid(char *name)
{
    Group *group = g_hash_table_lookup(state.groups, name);

    return group->gid;
}

uid_t get_avalable_gid()
{
    return get_avalable_id(&get_groups_keys, &get_group_uid);
}
