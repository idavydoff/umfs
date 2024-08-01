#include <grp.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void free_group(struct Group *group) {
    if (group) {
        free(group->name);
        free(group->password);

		for (int i = 0; i < group->members_count; i++) {
			free(group->members[i]);
		}
		free(group->members);

        free(group);
    }
}

void get_groups() {
	int capacity = 20;
	if (state.groups) {
		capacity = state.groups_count;
		for (int i = 0; i < state.groups_count; i++) {
			free_group(state.groups[i]);
		}
		free(state.groups);
	}

	state.groups = malloc(capacity * sizeof(struct Group*));
    state.groups_count = 0;

	struct group *grp;
	while ((grp = getgrent()) != NULL) {
		if (state.groups_count >= capacity) {
            capacity += 20;
            state.groups = realloc(state.groups, capacity * sizeof(struct Group*));
        }

		struct Group *new_group = malloc(sizeof(struct Group));

		new_group->gid = grp->gr_gid;
		new_group->name = strdup(grp->gr_name);
		new_group->password = strdup(grp->gr_passwd);

		int members_capacity = 10;
		new_group->members = malloc(members_capacity * sizeof(char*));
		new_group->members_count = 0;

		while (*grp->gr_mem) {
			if (new_group->members_count >= members_capacity) {
				members_capacity += 10;
				new_group->members = realloc(new_group->members, members_capacity * sizeof(char*));
			}
			new_group->members[new_group->members_count] = strdup(grp->gr_mem[new_group->members_count]);

			grp->gr_mem++;
			new_group->members_count++;
		}

        state.groups[state.groups_count] = new_group;
        state.groups_count += 1;

    }
    endgrent();
}