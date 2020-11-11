/** @file mutable_string.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of data types and functions implementing a mutable string.
 *
 * @author František Nečas  (xnecas27), FIT BUT
 */

#ifndef _MUTABLE_STRING_H
#define _MUTABLE_STRING_H 1

#include <stdlib.h>
#include <stdbool.h>

typedef struct mutable_string {
    char *array;
    size_t used;
    size_t size;
} MutableString;

/** @brief Initializes a mutable string.
 *
 * @param string The mutable string to initialize.
 * @param initial_size The size to initialize the inner array to. Must not be 0.
 * @return Whether the initialization was successful.
 */
bool mstr_init(MutableString *string, size_t initial_size);

/** @brief Returns a pointer to string data. */
char *mstr_content(const MutableString *string);

/** @brief Returns the current length of the string. */
size_t mstr_length(const MutableString *string);

/** @brief Inserts a new element at the end of the string.
 *
 * Inserts a new element at the end of the string. If the string is full,
 * its size is doubled. If doubling fails, expanding by 1 is used as a fallback.
 *
 * @param string The string to be inserted into.
 * @param new_element The element to insert.
 * @pre The input string has been initialized.
 * @post The string remains unchanged in case of failure.
 * @return Whether the insertion was successful. Returning false signals that
 *         reallocation of the array was needed but it failed (e.g. due to running
 *         out of space on the heap).
 */
bool mstr_append(MutableString *string, char new_element);

/** @brief Concatenates 2 mutable strings into one.
 *
 * Concatenates left_source and right_source and saves the result to
 * target MutableString.
 *
 * @param left_source First part of the result string.
 * @param right_source Second part of the result string.
 * @param target Where the result shall be stored. If it contains any data,
 *               it will be removed.
 * @return Whether the concatenation was successful. The operation may fail due
 *         the need to allocate more memory.
 */
bool mstr_concat(MutableString *target, const MutableString *left_source,
                 const MutableString *right_source);

/** @brief Destroys a mutable string.
 *
 * @param string The string to destroy.
 */
void mstr_free(MutableString *string);

#endif
