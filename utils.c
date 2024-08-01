#include <string.h>
#include <stdbool.h>

int get_dynamic_string_size(char *str) {
    int res = 0;

    while(*str) {
        res += sizeof(char);
        str++;
    }

    return res;
}

bool startsWith(const char *a, const char *b) {
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

int string_ends_with(const char *str, const char *suffix) {
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return 
    (str_len >= suffix_len) &&
    (0 == strcmp(str + (str_len-suffix_len), suffix));
}