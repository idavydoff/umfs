#include "common.h"
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    return (str_len >= suffix_len) && (0 == strcmp(str + (str_len - suffix_len), suffix));
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

uid_t get_avalable_id(GList *(*get_keys)(), uid_t (*get_instance_id_by_name)(char *name))
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