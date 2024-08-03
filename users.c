#include <pwd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void free_user(struct User *user) {
    if (user) {
        free(user->name);
        free(user->shell);
        free(user->dir);
        free(user);
    }
}

void get_users() {
    int capacity = 20;
    if (state.users) {
        capacity = state.users_count;
        for (int i = 0; i < state.users_count; i++) {
            free_user(state.users[i]);
        }
        free(state.users);
    }

    state.users = malloc(capacity * sizeof(struct User*));
    state.users_count = 0;

    struct passwd *p;
    while((p = getpwent()) != NULL) {
        if (state.users_count >= capacity) {
            capacity += 20;
            state.users = realloc(state.users, capacity * sizeof(struct User*));
        }

        struct User *new_user = malloc(sizeof(struct User));

        new_user->name = strdup(p->pw_name);
        new_user->uid = p->pw_uid;
        new_user->dir = strdup(p->pw_dir);
        new_user->shell = strdup(p->pw_shell);

        state.users[state.users_count] = new_user;
        state.users_count += 1;
    }
    endpwent();
}