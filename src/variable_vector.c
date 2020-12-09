/** @file variable_vector.c
 *
 * IFJ20 compiler
 *
 * @brief Contains definitions of variable vector functions.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#include <stdlib.h>
#include "variable_vector.h"
#include "symtable.h"
#include "stderr_message.h"

/** @brief Initializes the vector. */
void vv_init(VariableVector *vector) {
    vector->first = NULL;
}

/** @brief Adds a variable to the variable vector. */
bool vv_append(VariableVector *vector, VariableData var) {
    Variable *new = calloc(sizeof(Variable), 1);
    if (new == NULL) {
        stderr_message("variable_vector", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
        return false;
    }
    new->data = var;
    new->next = vector->first;
    vector->first = new;
    return true;
}

/** @brief Checks whether the vector contains the given symbol. */
Variable *vv_find(VariableVector *vector, STSymbol *symbol) {
    Variable *curr = vector->first;
    while (curr != NULL) {
        if (curr->data.symbol == symbol) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/** @brief Removes the given symbol from the vector. */
void vv_remove_symbol(VariableVector *vector, STSymbol *symbol) {
    Variable *prev = NULL;
    Variable *curr = vector->first;
    while (curr != NULL) {
        if (curr->data.symbol == symbol) {
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    if (curr == NULL) {
        return;
    }
    if (prev == NULL) {
        // Curr was head
        vector->first = curr->next;
    } else {
        prev->next = curr->next;
    }
    free(curr);
}

/** @brief Destroys the variable vector. */
void vv_free(VariableVector *vector) {
    Variable *curr = vector->first;
    Variable *tmp;
    while (curr != NULL) {
        tmp = curr->next;
        free(curr);
        curr = tmp;
    }
}
