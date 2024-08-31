#include <glib.h>
#include <stdbool.h>

#ifndef COMMON /* Include guard */
#define COMMON

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

#endif
