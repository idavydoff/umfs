#include <grp.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "common.h"

void free_group(struct Group *group)
{
    if (group)
    {
        free(group->name);
        group->name = NULL;
        free(group->password);
        group->password = NULL;

        for (int i = 0; i < group->members_count; i++)
        {
            free(group->members[i]);
            group->members[i] = NULL;
        }
        free(group->members);
        group->members = NULL;

        free(group);
        group = NULL;
    }
}

void get_groups()
{
    if (state.groups)
    {

        GList *keys = g_hash_table_get_keys(state.groups);
        GList *keys_ptr = keys;

        while (keys_ptr)
        {
            Group *group = g_hash_table_lookup(state.groups, keys_ptr->data);
            free_group(group);
            free(keys_ptr->data);
            keys_ptr = keys_ptr->next;
        }

        g_list_free(keys);
        g_hash_table_steal_all(state.groups);
    }
    else
    {
        state.groups = g_hash_table_new(g_str_hash, g_str_equal);
    }

    struct group *grp;
    while ((grp = getgrent()) != NULL)
    {

        struct Group *new_group = malloc(sizeof(struct Group));

        new_group->gid = grp->gr_gid;
        new_group->name = strdup(grp->gr_name);
        new_group->password = strdup(grp->gr_passwd);

        int members_capacity = 10;
        new_group->members = malloc(members_capacity * sizeof(char *));
        new_group->members_count = 0;

        while (*grp->gr_mem)
        {
            if (new_group->members_count >= members_capacity)
            {
                members_capacity += 10;
                new_group->members = realloc(new_group->members, members_capacity * sizeof(char *));
            }
            new_group->members[new_group->members_count] = strdup(grp->gr_mem[new_group->members_count]);

            grp->gr_mem++;
            new_group->members_count++;
        }

        g_hash_table_insert(state.groups, strdup(grp->gr_name), new_group);
    }
    endgrent();
}