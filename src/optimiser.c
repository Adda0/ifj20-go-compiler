/** @file optimiser.c
 *
 * IFJ20 compiler
 *
 * @brief Implements the AST optimiser.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#include <stdbool.h>
#include "optimiser.h"
#include "control_flow.h"
#include "ast.h"
#include "stderr_message.h"

void optimise_add(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    ASTNode *right_op = (*ast)->right;
    if (left_op->actionType == AST_CONST_INT && right_op->actionType == AST_CONST_INT) {
        int64_t new_val = left_op->data[0].intConstantValue + right_op->data[0].intConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_consti(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    }
}

void optimise_ast(ASTNode **ast, bool *changed) {
    if (*ast == NULL) {
        return;
    }
    switch ((*ast)->actionType) {
        case AST_ADD:
            optimise_add(ast, changed);
            break;
        default: break;
    }
    if ((*ast)->actionType == AST_LIST) {
        // Walk through all elements of the list
        for (unsigned i = 0; i < (*ast)->dataCount; i++) {
            optimise_ast(&(*ast)->data[i].astPtr, changed);
        }
    } else {
        optimise_ast(&(*ast)->left, changed);
        optimise_ast(&(*ast)->right, changed);
    }
}

void optimise_expressions(CFStatement *stat, bool *changed) {
    if (stat != NULL && !is_statement_empty(stat) && stat->followingStatement != NULL &&
        stat->followingStatement->statementType != CF_IF) {
        switch (stat->statementType) {
            case CF_BASIC:
            case CF_RETURN:
                optimise_ast(&stat->data.bodyAst, changed);
                break;
            case CF_IF:
                optimise_ast(&stat->data.ifData->conditionalAst, changed);
                optimise_expressions(stat->data.ifData->thenStatement, changed);
                optimise_expressions(stat->data.ifData->elseStatement, changed);
                break;
            case CF_FOR:
                optimise_ast(&stat->data.forData->definitionAst, changed);
                optimise_ast(&stat->data.forData->conditionalAst, changed);
                optimise_ast(&stat->data.forData->afterthoughtAst, changed);
                optimise_expressions(stat->data.forData->bodyStatement, changed);
                break;
        }
    }

    if (stat->followingStatement != NULL) {
        optimise_expressions(stat->followingStatement, changed);
    }
}

void fold_constants(bool *changed) {
    CFProgram *prog = get_program();
    CFFuncListNode *n = prog->functionList;
    while (n != NULL) {
        optimise_expressions(n->fun.rootStatement, changed);
        n = n->next;
    }
}

void optimiser_optimise() {
    while (true) {
        bool changed = false;
        fold_constants(&changed);
        if (!changed) {
            break;
        }
    }
}