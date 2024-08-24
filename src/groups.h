#include "common.h"
#include <sys/types.h>

#ifndef GROUPS /* Include guard */
#define GROUPS

void free_group(struct User *user);
void get_groups();
uid_t get_avalable_gid();
Group *copy_group(Group *source_group);

#endif
