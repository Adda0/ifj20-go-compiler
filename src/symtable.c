/** @file symtable.c
 *
 * IFJ20 compiler
 *
 * @brief Contains implementation of symbol table as hash table.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#include <string.h>
#include <stdlib.h>

#include "symtable.h"
#include "stderr_message.h"

size_t symtable_hash(const char *key) {
    unsigned h = 0;
    const unsigned char *p;
    for (p = (const unsigned char*) key; *p != '\0'; p++) {
        h = 65599 * h + *p;
    }

    return h;
}

SymbolTable *symtable_init(size_t n) {
    SymbolTable *table = (SymbolTable *) calloc(1, sizeof(SymbolTable) + sizeof(STItem) * n);
    if (table == NULL) {
        stderr_message("symbol_table", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Allocation of symbol table failed.\n");
        return NULL;
    }

    table->arr_size = n;
    table->size = 0;

    for (size_t i = 0; i < n; i++) {
        table->arr[i] = NULL;
    }

    return table;
}

STItem *symtable_find(SymbolTable *table, const char *key) {
    STItem *item = table->arr[symtable_hash(key) % table->arr_size];

    while (item != NULL) {
        if (strcmp(item->key, key) == 0) {
            return item;
        }
        item = item->next;
    }
    return NULL;
}

STItem *symtable_add(SymbolTable *table, const char *key, STType type) {
    STItem *item = symtable_find(table, key);
    if (item != NULL) { // item of given key already exists, this should not happen
        stderr_message("symbol_table", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "The item with the key '%s' added to the symbol table already exists.\n", key);
        return NULL;
    }

    size_t i = symtable_hash(key) % table->arr_size;


    STItem *new = (STItem *) malloc(sizeof(STItem));
    if (new == NULL) {
        stderr_message("symbol_table", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Malloc of a new item for symbol table failed.\n");
        return NULL;
    }

    new->key = (char *) malloc(sizeof(char) * (strlen(key) + 1));
    if (new->key == NULL) {
        free(new);
        new = NULL;
        stderr_message("symbol_table", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Malloc for a key in a new item for symbol table failed.\n");
        return NULL;
    }
    strcpy(new->key, key);

    new->data.identifier = (char *) malloc(sizeof(char) * (strlen(key) + 1));
    if (new->data.identifier == NULL) {
        free(new->key);
        new->key = NULL;
        free(new);
        new = NULL;
        stderr_message("symbol_table", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Malloc for an identifier in a new item for symbol table failed.\n");
        return NULL;
    }
    strcpy((char *) new->data.identifier, key);

    new->next = table->arr[i];
    new->data.type = type;

    if (type == ST_SYMBOL_FUNC) {
        new->data.data.func_data.ret_types = NULL;
        new->data.data.func_data.params = NULL;
        new->data.data.func_data.defined = false;
    } else {
        new->data.data.var_data.type = CF_UNKNOWN;
    }


    table->arr[i] = new;
    table->size++;

    return new;
}

void symtable_free(SymbolTable *table) {
    STItem *tmp = NULL;
    STItem *tmp_next = NULL;
    for (size_t i = 0; i < table->arr_size; i++) {
        tmp = table->arr[i];
        while (tmp != NULL) {
            tmp_next = tmp->next;

            free(tmp->key);
            tmp->key = NULL;

            free((char *) tmp->data.identifier);
            tmp->data.identifier = NULL;

            if (tmp->data.type == ST_SYMBOL_FUNC) {

                STParam *param = tmp->data.data.func_data.params;
                STParam *param_to_delete = NULL;
                while (param != NULL) {
                    param_to_delete = param;
                    param = param->next;
                    free(param_to_delete->id);
                    param_to_delete->id = NULL;

                    free(param_to_delete);
                    param_to_delete = NULL;
                }

                STParam *ret_type = tmp->data.data.func_data.ret_types;
                STParam *ret_type_to_delete = NULL;
                while (ret_type != NULL) {
                    ret_type_to_delete = ret_type;
                    ret_type = ret_type->next;
                    free(ret_type_to_delete->id);
                    ret_type_to_delete->id = NULL;

                    free(ret_type_to_delete);
                    ret_type_to_delete = NULL;
                }
            }

            free(tmp);
            tmp = NULL;
            tmp = tmp_next;
        }
    }

    free(table);
    table = NULL;
}

bool symtable_add_param(STItem *item, const char *id, STDataType type) {
    STParam *new = (STParam *) malloc(sizeof(STParam));
    if (new == NULL) {
        stderr_message("symbol_table", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Malloc for a new parameter failed.\n");
        return false;
    }

    new->next = NULL;
    new->type = type;
    if (id != NULL) {
        new->id = (char *) malloc(sizeof(char) * (strlen(id) + 1));
        if (new->id == NULL) {
            free(new);
            new = NULL;
            return false;
        }
        strcpy(new->id, id);
    } else {
        new->id = NULL;
    }

    if (item->data.data.func_data.params == NULL) {
        item->data.data.func_data.params = new;
    } else {
        STParam *tmp = item->data.data.func_data.params;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = new;
    }

    item->data.data.func_data.params_count++;
    return true;
}

bool symtable_add_ret_type(STItem *item, const char *id, STDataType type) {
    STParam *new = (STParam *) malloc(sizeof(STParam));
    if (new == NULL) {
        stderr_message("symbol_table", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Malloc for a new parameter failed.\n");
        return false;
    }

    new->next = NULL;
    new->type = type;

    if (id != NULL) {
        new->id = (char *) malloc(sizeof(char) * (strlen(id) + 1));
        if (new->id == NULL) {
            free(new);
            new = NULL;
            return false;
        }
        strcpy(new->id, id);
    } else {
        new->id = NULL;
    }

    if (item->data.data.func_data.ret_types == NULL) {
        item->data.data.func_data.ret_types = new;
    } else {
        STParam *tmp = item->data.data.func_data.ret_types;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = new;
    }

    item->data.data.func_data.ret_types_count++;
    return true;
}

STItem *symtable_get_first_item(SymbolTable *table) {
    for (size_t i = 0; i < table->arr_size; i++) {
        if (table->arr[i] != NULL) {
            return table->arr[i];
        }
    }

    return NULL;
}

STItem *symtable_get_next_item(SymbolTable *table, STItem *current_item) {
    size_t index = symtable_hash(current_item->key) % table->arr_size;

    if (current_item->next != NULL) {
        return current_item->next;
    } else {
        for (size_t i = index + 1; i < table->arr_size; i++) {
            if (table->arr[i] != NULL) {
                return table->arr[i];
            }
        }
    }

    return NULL;
}
