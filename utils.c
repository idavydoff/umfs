#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

int get_dynamic_string_size(char *str)
{
    int res = 0;

    while (*str)
    {
        res += sizeof(char);
        str++;
    }

    return res;
}

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

    return (str_len >= suffix_len) &&
           (0 == strcmp(str + (str_len - suffix_len), suffix));
}

char *
get_item_name_from_path(const char *path, char *offset)
{
    char *path_dup = strdup(path);
    char *original_path_dup = path_dup;

    path_dup += strlen(offset);
    int len = strlen(path_dup);
    static char result[50];

    for (int i = 0; i < len; i++)
    {
        if (path_dup[i] == '/')
        {
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
