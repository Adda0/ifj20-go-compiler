/** @file symtable.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of functions and data types for the symbol table module.
 *
 * @author František Nečas (xnecas27), FIT BUT
 * @author Ondřej Ondryáš  (xondry02), FIT BUT
 */

#ifndef _SYMTABLE_H
#define _SYMTABLE_H 1

#include <string.h>
#include <stdbool.h>

typedef enum cfgraph_data_type {
    CF_INT,
    CF_FLOAT,
    CF_STRING,
    CF_BOOL,
    CF_UNKNOWN,
    CF_NIL
} STDataType;

typedef enum st_type {
    ST_SYMBOL_VAR,
    ST_SYMBOL_FUNC,
} STType;

/** A structure representing a parameter or a return type. */
typedef struct st_param {
    char *id;         /**< Id of the parameter (can be NULL if it is unnamed return type). */
    STDataType type;  /**< Type of the parameter. */
    struct st_param *next;
} STParam;

/** A structure representing data for a symbol of type function. */
typedef struct st_function_data {
    STParam *params;     /**< Pointer to first param. */
    STParam *ret_types;  /**< Pointer to first return type. */
    bool defined;        /**< Whether the function has been defined */
} STFunctionData;

/** A structure representing data for a symbol of type variable. */
typedef struct st_variable_data {
    STDataType type;  /**< Type of the variable. */
} STVariableData;

/** A union containing symbol data. */
typedef union st_symbol_data {
    STVariableData var_data;  /**< Data for a symbol of type variable. */
    STFunctionData func_data; /**< Data for a symbol of type function. */
} STSymbolData;

/** A structure representing a symbol. */
typedef struct st_symbol {
    STType type;                /**< Type of the symbol (function or variable). */
    const char *identifier;     /**< Identifier of the variable. */
    unsigned reference_counter; /**< Counter of symbol usages. */
    STSymbolData data;          /**< Data of the symbol. */
} STSymbol;

/** A structure representing an item in the symbol table. */
typedef struct st_item {
    char *key;                  /**< Pointer to allocated key. */
    STSymbol data;              /**< Data of the item. */
    struct st_item *next;       /**< Pointer to next entry in the linked list. */
} STItem;

/** A structure representing a symbol table. */
typedef struct symbol_table {
    unsigned size;      /**< Number of items in the symbol table. */
    unsigned arr_size;  /**< Number of elements in arr. */
    STItem *arr[];      /**< An array of pointers to entries in the table. */
} SymbolTable;

/** @brief Hashing function.
 *
 * Calculates the index in the hash table.
 *
 * @param key Key in the hash table.
 * @return The calculated index.
 */
size_t symtable_hash(const char *key);

/** @brief Symbol table constructor.
 *
 * @param n Size of the inner array of pointers
 * @return Pointer to the initialized symbol table, NULL if allocation failed.
 */
SymbolTable *symtable_init(size_t n);

/** @brief Searches in the symbol table.
 *
 * @param table Table to search in.
 * @param key Key to search.
 * @return Pointer to the found item. NULL if the item wasn't found.
 */
STItem *symtable_find(SymbolTable *table, const char *key);

/** @brief Adds a new element to the symbol table.
 *
 * @param table Table to add to.
 * @param key Key to add.
 * @param type Type of the new symbol.
 * @return Pointer to the new item. NULL if allocation failed.
 */
STItem *symtable_add(SymbolTable *table, const char *key, STType type);

/** @brief Destroys the symbol table.
 *
 * @param table Table to destroy.
 * @post All memory allocated by the symbol table has been freed.
 */
void symtable_free(SymbolTable *table);

/** @brief Adds a new parameter to function element in symbol table.
 *
 * @param item Symbol to add to.
 * @param id Id of the parameter to be added.
 * @param type Type of the parameter to be added.
 * @return True if the parameter was added successfully, false otherwise.
 */
bool symtable_add_param(STItem *item, const char *id, STDataType type);

/** @brief Adds a new return type to function element in symbol table.
 *
 * @param item Symbol to add to.
 * @param id Id of the return type to be added.
 * @param type Type of the return type to be added.
 * @return True if the return type was added successfully, false otherwise.
 */
bool symtable_add_ret_type(STItem *item, const char *id, STDataType type);

#endif
