/** @file mutable_string.c
 *
 * IFJ20 compiler
 *
 * @brief Contains implementation of mutable string data structure.
 *
 * @author František Nečas  (xnecas27), FIT BUT
 */

#include <stdlib.h>
#include <stdbool.h>
#include "mutable_string.h"


bool mstr_init(MutableString *string, size_t initial_size) {
    if (initial_size == 0) {
        return false;
    }
    string->array = (char *) malloc(initial_size * sizeof(char));
    if (string->array == NULL) {
        return false;
    }
    string->array[0] = '\0';
    string->used = 0;
    string->size = initial_size;
    return true;
}

char *mstr_content(MutableString *string) {
    return string->array;
}

size_t mstr_length(MutableString *string) {
    return string->used;
}

bool mstr_append(MutableString *string, char new_element) {
    if (string->used + 1 == string->size) {
        // The array is currently full, increase its size.
        char *new_arr = realloc(string->array, 2 * string->size);
        if (new_arr == NULL) {
            new_arr = realloc(string->array, string->size + 1);
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

void mstr_free(MutableString *string) {
    free(string->array);
}
