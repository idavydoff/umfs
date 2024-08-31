#include "umfs.h"
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

bool startsWith(const char *a, const char *b)
{
    if (strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

int string_ends_with(const char *str, const char *suffix)
{
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);

    return (str_len >= suffix_len)
        && (0 == strcmp(str + (str_len - suffix_len), suffix));
}

char *get_item_name_from_path(const char *path, char *offset)
{
    char *path_dup = strdup(path);
    char *original_path_dup = path_dup;

    path_dup += strlen(offset);
    int len = strlen(path_dup);
    static char result[50];

    for (int i = 0; i < len; i++) {
        if (path_dup[i] == '/') {
            strncpy(result, path_dup, i);
            result[i] = '\0';

            free(original_path_dup);
            return result;
        }
    }
    strcpy(result, path_dup);

    free(original_path_dup);

    return result;
}

char *get_path_end(const char *path)
{
    char *path_dup = strdup(path);

    static char *res = NULL;
    res = malloc(sizeof(path) * sizeof(char));

    int last_slash_index = 0;

    for (int i = 0; i < strlen(path_dup); i++) {
        if (path_dup[i] == '/') {
            last_slash_index = i;
        }
    }

    char *end = path_dup + last_slash_index + 1;

    res = realloc(res, (strlen(end) * sizeof(char)) + 1);
    strcpy(res, end);

    free(path_dup);

    return res;
}

uid_t get_avalable_id(
    GList *(*get_keys)(), uid_t (*get_instance_id_by_name)(char *name))
{
    uid_t result_id = 1000;

    GList *keys = get_keys();
    GList *keys_ptr = keys;

    // Используется как HashSet
    GHashTable *ids = g_hash_table_new(g_str_hash, g_str_equal);

    while (keys_ptr) {
        uid_t instance_id = get_instance_id_by_name(keys_ptr->data);

        char id_string[6];
        sprintf(id_string, "%d", instance_id);

        g_hash_table_insert(ids, strdup(id_string), NULL);

        keys_ptr = keys_ptr->next;
    }

    g_list_free(keys);

    bool found = true;

    while (found) {
        char id_string[6];
        sprintf(id_string, "%d", result_id);

        if (g_hash_table_contains(ids, id_string)) {
            result_id++;
        } else {
            found = false;
        }
    }

    // Free ids
    keys = g_hash_table_get_keys(ids);
    keys_ptr = keys;
    while (keys_ptr) {
        free(keys_ptr->data);
        keys_ptr = keys_ptr->next;
    }
    g_list_free(keys);
    g_hash_table_destroy(ids);

    return result_id;
}

void dynamic_strcat(char *str_a, char *str_b)
{
    int str_a_len = strlen(str_a);
    int str_b_len = strlen(str_b);

    for (int i = 0; i < str_b_len; i++) {
        str_a[str_a_len + i] = str_b[i];
        str_a[str_a_len + i + 1] = '\0';
    }
}

int save_users_to_file()
{
    struct stat passwd_stat;
    stat("/etc/passwd", &passwd_stat);

    char *new_content = malloc(passwd_stat.st_size + 50);
    *new_content = '\0';

    GList *users_list = g_hash_table_get_values(state.users);
    GList *users_list_ptr = users_list;

    while (users_list_ptr) {
        User *user = users_list_ptr->data;
        char tmp[200];
        sprintf(tmp, "%s:x:%d:%d:%s:%s:%s\n", user->name, user->uid, user->gid,
            user->gecos, user->dir, user->shell);
        dynamic_strcat(new_content, tmp);
        users_list_ptr = users_list_ptr->next;
    }

    g_list_free(users_list);

    FILE *fp = fopen("/etc/passwd", "w");
    if (fp == NULL) {
        free(new_content);
        return -EIO;
    }

    fputs(new_content, fp);
    fclose(fp);

    free(new_content);

    return 0;
}

int save_groups_to_file()
{
    struct stat group_stat;
    stat("/etc/group", &group_stat);

    char *new_content = malloc(group_stat.st_size + 50);
    *new_content = '\0';

    GList *groups_list = g_hash_table_get_values(state.groups);
    GList *groups_list_ptr = groups_list;

    while (groups_list_ptr) {
        Group *group = groups_list_ptr->data;
        char *tmp = malloc(200 * sizeof(char));
        tmp[0] = '\0';
        sprintf(tmp, "%s:x:%d:", group->name, group->gid);

        for (int i = 0; i < group->members_count; i++) {
            User *user = g_hash_table_lookup(state.users, group->members[i]);

            if (user->gid == group->gid) {
                continue;
            }

            char *tmp_member = malloc(50 * sizeof(char));
            tmp_member[0] = '\0';
            if (i > 0) {
                tmp_member[0] = ',';
                tmp_member[1] = '\0';
            }
            dynamic_strcat(tmp_member, group->members[i]);

            dynamic_strcat(tmp, tmp_member);
            free(tmp_member);
        }

        dynamic_strcat(tmp, "\n");

        dynamic_strcat(new_content, tmp);
        free(tmp);
        groups_list_ptr = groups_list_ptr->next;
    }

    g_list_free(groups_list);

    FILE *fp = fopen("/etc/group", "w");
    if (fp == NULL) {
        return -EIO;
    }

    fputs(new_content, fp);
    fclose(fp);

    free(new_content);

    return 0;
}

short delete_from_dynamic_string_array(
    char ***arr1, short arr_length, char *string)
{
    char **arr = *arr1;

    char **tmp = malloc(arr_length * sizeof(char *));
    short deleted_count = 0;
    for (short i = 0; i < arr_length; i++) {
        if (strcmp(arr[i], string) == 0) {
            deleted_count++;
            free(arr[i]);
            continue;
        }
        tmp[i - deleted_count] = strdup(arr[i]);
        free(arr[i]);
    }

    free(*arr1);

    if (deleted_count)
        tmp = realloc(tmp, (arr_length - deleted_count) * sizeof(char *));

    *arr1 = tmp;

    return deleted_count;
}