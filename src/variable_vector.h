/** @file variable_vector.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of functions and data types required for vector of variables
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#ifndef _VARIABLE_VECTOR_H
#define _VARIABLE_VECTOR_H 1

#include <stdlib.h>
#include "symtable.h"
#include "ast.h"

typedef struct variable_data {
    STSymbol *symbol;
    ASTNodeType type;
    ASTNodeData data;
} VariableData;

typedef struct variable {
    VariableData data;
    struct variable *next;
} Variable;

typedef struct variable_vector {
    Variable *first;
} VariableVector;

/** @brief Initializes the vector. */
void vv_init(VariableVector *vector);

/** @brief Adds a variable to the variable vector. */
bool vv_append(VariableVector *vector, VariableData var);

/** @brief Checks whether the vector contains the given symbol. */
Variable *vv_find(VariableVector *vector, STSymbol *symbol);

/** @brief Removes the given symbol from the vector. */
void vv_remove_symbol(VariableVector *vector, STSymbol *symbol);

/** @brief Destroys the variable vector. */
void vv_free(VariableVector *vector);

#endif
