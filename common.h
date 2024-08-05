#include <stdbool.h>

#ifndef COMMOn   /* Include guard */
#define COMMOn

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }

typedef struct User {
    char *name;
    uid_t uid;
    char *shell;
    char *dir;
}User;

typedef struct State {
    struct User **users;
    int users_count;
    struct Group **groups;
    int groups_count;
}State;

typedef struct Group {
    char *name;
    uid_t gid;
    char *password;
    char **members;
    int members_count;
}Group;

extern struct State state;
extern pthread_mutex_t state_data_mutex;

int get_dynamic_string_size(char *str);
bool startsWith(const char *a, const char *b);
int string_ends_with(const char *str, const char *suffix);

#endif