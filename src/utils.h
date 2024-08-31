#include <glib.h>
#include <stdbool.h>

#ifndef UTILS /* Include guard */
#define UTILS

bool startsWith(const char *a, const char *b);
int string_ends_with(const char *str, const char *suffix);
char *get_item_name_from_path(const char *path, char *offset);
char *get_path_end(const char *path);
uid_t get_avalable_id(
    GList *(*get_keys)(), uid_t (*get_instance_id_by_name)(char *name));
void dynamic_strcat(char *str_a, char *str_b);
int save_users_to_file();
int save_groups_to_file();
short delete_from_dynamic_string_array(
    char ***arr1, short arr_length, char *string);

#endif
