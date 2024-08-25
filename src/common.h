#include <glib.h>
#include <stdbool.h>

#ifndef COMMON /* Include guard */
#define COMMON

#define OPTION(t, p) { t, offsetof(struct options, p), 1 }

typedef struct User {
    char *name;
    char *gecos;
    uid_t uid;
    uid_t gid;
    char *shell;
    char *dir;
    char **groups;
    int groups_count;
    bool sudo;
} User;

typedef struct Group {
    char *name;
    uid_t gid;
    char **members;
    int members_count;
    bool primary_for_some_users;
} Group;

typedef struct State {
    GHashTable *users;
    GHashTable *groups;
    char *fake_file_path;
} State;

extern struct State state;
extern pthread_mutex_t state_data_mutex;
extern char ABSOLUTE_MOUNT_PATH[PATH_MAX];

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
