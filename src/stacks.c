/** @file stacks.c
 *
 * IFJ20 compiler
 *
 * @brief Contains definitions of various stacks
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#include <stdlib.h>
#include <stdbool.h>
#include "scanner.h"
#include "precedence_parser.h"
#include "stacks.h"

void precedence_stack_init(PrecedenceStack *stack) {
    stack->top = NULL;
}

bool precedence_stack_push(PrecedenceStack *stack, Token token) {
    PrecedenceNode *node = malloc(sizeof(PrecedenceNode));
    if (node == NULL) {
        return false;
    }
    if (stack->top != NULL) {
        stack->top->rptr = node;
    }
    node->data = token;
    node->lptr = stack->top;
    node->rptr = NULL;
    stack->top = node;
    return true;
}

bool precedence_stack_post_insert(PrecedenceStack *stack, PrecedenceNode *node, Token token) {
    PrecedenceNode *new_node = malloc(sizeof(PrecedenceNode));
    if (new_node == NULL) {
        return false;
    }
    new_node->data = token;
    new_node->lptr = node;
    new_node->rptr = node->rptr;
    node ->rptr = new_node;
    if (node == stack->top) {
        stack->top = new_node;
    } else {
        new_node->rptr->lptr = new_node;
    }
    return true;
}

PrecedenceNode *precedence_stack_top(PrecedenceStack *stack) {
    PrecedenceNode *current = stack->top;
    while (current != NULL) {
        if (current->data.type != SYMB_BEGIN && current->data.type != SYMB_NONTERMINAL) {
            return current;
        }
        current = current->lptr;
    }
    return NULL;
}

PrecedenceNode *precedence_stack_reduce_start(PrecedenceStack *stack) {
    PrecedenceNode *current = stack->top;
    while (current != NULL) {
        if (current->data.type == SYMB_BEGIN) {
            return current;
        }
        current = current->lptr;
    }
    return NULL;
}

void precedence_stack_dispose(PrecedenceStack *stack) {
    PrecedenceNode *current = stack->top;
    while (current != NULL) {
        PrecedenceNode *next = current->lptr;
        free(current);
        current = next;
    }
}

void precedence_stack_pop_from(PrecedenceStack *stack, PrecedenceNode *pop_from) {
    stack->top = pop_from->lptr;
    stack->top->rptr = NULL;
    while (pop_from != NULL) {
        PrecedenceNode *next = pop_from->rptr;
        free(pop_from);
        pop_from = next;
    }
}