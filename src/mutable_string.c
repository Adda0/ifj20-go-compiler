/** @file mutable_string.c
 *
 * IFJ20 compiler
 *
 * @brief Implements the mutable string data structure.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mutable_string.h"


bool mstr_init(MutableString *string, size_t initial_size) {
    string->array = NULL;
    string->used = 0;
    string->size = initial_size;
    if (initial_size == 0) {
        return false;
    }
    string->array = (char *) malloc(sizeof(char) * initial_size);
    if (string->array == NULL) {
        return false;
    }
    string->array[0] = '\0';
    return true;
}

char *mstr_content(const MutableString *string) {
    return string->array;
}

size_t mstr_length(const MutableString *string) {
    return string->used;
}

bool mstr_append(MutableString *string, char new_element) {
    if (string->used + 1 == string->size) {
        // The array is currently full, increase its size.
        char *new_arr = (char *) realloc(string->array, 2 * string->size);
        if (new_arr == NULL) {
            new_arr = (char *) realloc(string->array, string->size + 1);
            if (new_arr == NULL) {
                return false;
            } else {
                string->size += 1;
            }
        } else {
            string->size *= 2;
        }
        string->array = new_arr;
    }
    string->array[string->used++] = new_element;
    string->array[string->used] = '\0'; // We replaced the previous \0, terminate the string.
    return true;
}

bool mstr_concat(MutableString *target, const MutableString *left_source,
                 const MutableString *right_source) {
    unsigned left_len = left_source->used;
    unsigned right_len = right_source->used;
    if (!mstr_init(target, left_len + right_len + 1)) {
        return false;
    }
    target->used = left_len + right_len;
    memcpy(target->array, left_source->array, left_len);
    memcpy(target->array + left_len, right_source->array, right_len + 1);
    return true;
}

void mstr_free(MutableString *string) {
    if (string->array != NULL) {
        free(string->array);
        string->array = NULL;
    }
}
