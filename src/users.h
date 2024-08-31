#include "umfs.h"
#include <sys/types.h>

#ifndef USERS /* Include guard */
#define USERS

void free_user(struct User *user);
void get_users();
uid_t get_avalable_uid();
User *copy_user(User *source_user);
char *get_user_primary_group_name(User *user);

#endif