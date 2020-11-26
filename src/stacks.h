/** @file stacks.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of functions and data types for various stack types.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#ifndef _STACKS_H
#define _STACKS_H 1

#include <stdbool.h>
#include "scanner.h"
#include "symtable.h"
#include "precedence_parser.h"

typedef struct precedence_node {
    StackSymbol data;
    struct precedence_node *lptr;
    struct precedence_node *rptr;
} PrecedenceNode;

typedef struct precedence_stack {
    PrecedenceNode *top;
} PrecedenceStack;

/** @brief Initializes the precedence stack. */
void precedence_stack_init(PrecedenceStack *stack);

/** @brief Pushes a new node onto a precedence stack.
 *
 * @param stack Stack to push onto.
 * @param data Data for the new node.
 * @return Whether the push was successful.
 */
bool precedence_stack_push(PrecedenceStack *stack, StackSymbol data);

/** @brief Inserts a new token after the given node.
 * 
 * @param stack Stack to insert into.
 * @param node Node to insert after.
 * @param data Data for the new node.
 * @return Whether the insert was successful.
 */
bool precedence_stack_post_insert(PrecedenceStack *stack, PrecedenceNode *node, StackSymbol data);

/** @brief Returns the top of the stack. */
PrecedenceNode *precedence_stack_top(PrecedenceStack *stack);

/** @brief Returns the top terminal on the stack.
 * 
 * Finds the first node from the top that doesn't contain special symbols
 * (such as <) or nonterminal and returns the node. The node represents where
 * rule reduction should start.
 */
PrecedenceNode *precedence_stack_reduce_start(PrecedenceStack *stack);

/** @brief Clears the whole stack. */
void precedence_stack_dispose(PrecedenceStack *stack);

/** @brief Clears the whole stack from pop_from to top. */
void precedence_stack_pop_from(PrecedenceStack *stack, PrecedenceNode *pop_from);

typedef struct symtable_node {
    SymbolTable *table;
    struct symtable_node *next;
} SymtableNode;

typedef struct symtable_stack {
    SymtableNode *top;
} SymtableStack;

/** @brief Initializes the symtable stack. */
void symtable_stack_init(SymtableStack *stack);

/** @brief Pushes a new symbol table onto the stack. */
SymtableNode *symtable_stack_push(SymtableStack *stack, SymbolTable *table);

/** @brief Returns the top of the symbol table stack. */
SymtableNode *symtable_stack_top(SymtableStack *stack);

/** @brief Searches for a symbol in symtable stack, returns the first occurrence. */
STItem *symtable_stack_find_symbol(SymtableStack *stack, const char *symbol);

/** @brief Removes the top symbol table from the stack. */
void symtable_stack_pop(SymtableStack *stack);

#endif
