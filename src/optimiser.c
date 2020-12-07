/** @file optimiser.c
 *
 * IFJ20 compiler
 *
 * @brief Implements the AST optimiser.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "optimiser.h"
#include "control_flow.h"
#include "ast.h"
#include "stderr_message.h"

static double dabs(double x) {
    return x > 0 ? x : -x;
}

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
    } else if (left_op->actionType == AST_CONST_FLOAT && right_op->actionType == AST_CONST_FLOAT) {
        double new_val = left_op->data[0].floatConstantValue + right_op->data[0].floatConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_constf(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else if (left_op->actionType == AST_CONST_STRING && right_op->actionType == AST_CONST_STRING) {
        const char *left = left_op->data[0].stringConstantValue;
        const char *right = right_op->data[0].stringConstantValue;
        char *new = malloc(strlen(left) + strlen(right) + 1);
        if (new == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        strcpy(new, left);
        strcat(new, right);
        clean_ast(*ast);
        *ast = ast_leaf_consts(new);
        free(new);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else {
        // If one operand is 0 or empty string, we can remove the addition
        ASTNode *target = NULL;
        if ((left_op->actionType == AST_CONST_INT && left_op->data[0].intConstantValue == 0) ||
                ((left_op->actionType == AST_CONST_FLOAT && dabs(left_op->data[0].floatConstantValue) < 1e-10)) ||
                ((left_op->actionType == AST_CONST_STRING && strlen(left_op->data[0].stringConstantValue) == 0))) {
            target = right_op;
            (*ast)->right = NULL;
        } else if ((right_op->actionType == AST_CONST_INT && right_op->data[0].intConstantValue == 0) ||
                (right_op->actionType == AST_CONST_FLOAT && dabs(right_op->data[0].floatConstantValue) < 1e-10) ||
                (right_op->actionType == AST_CONST_STRING && strlen(right_op->data[0].stringConstantValue) == 0)) {
            target = left_op;
            (*ast)->left = NULL;
        }
        if (target != NULL) {
            clean_ast(*ast);
            *ast = target;
            *changed = true;
        }
    }
}

void optimise_subtract(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    ASTNode *right_op = (*ast)->right;
    if (left_op->actionType == AST_CONST_INT && right_op->actionType == AST_CONST_INT) {
        int64_t new_val = left_op->data[0].intConstantValue - right_op->data[0].intConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_consti(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else if (left_op->actionType == AST_CONST_FLOAT && right_op->actionType == AST_CONST_FLOAT) {
        double new_val = left_op->data[0].floatConstantValue - right_op->data[0].floatConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_constf(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else {
        // If right operand is 0, we can remove the subtraction
        if ((right_op->actionType == AST_CONST_INT && right_op->data[0].intConstantValue == 0) ||
                (right_op->actionType == AST_CONST_FLOAT && dabs(right_op->data[0].floatConstantValue) < 1e-10)) {
            (*ast)->left = NULL;
            clean_ast(*ast);
            *ast = left_op;
            *changed = true;
        }
    }
}

void optimise_multiply(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    ASTNode *right_op = (*ast)->right;
    if (left_op->actionType == AST_CONST_INT && right_op->actionType == AST_CONST_INT) {
        int64_t new_val = left_op->data[0].intConstantValue * right_op->data[0].intConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_consti(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else if (left_op->actionType == AST_CONST_FLOAT && right_op->actionType == AST_CONST_FLOAT) {
        double new_val = left_op->data[0].floatConstantValue * right_op->data[0].floatConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_constf(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else {
        // If one of the operands is 0, the operation is also constant. If one of the operands is 1,
        // we can remove the multiplication.
        ASTNode *target = NULL;
        if ((right_op->actionType == AST_CONST_INT && right_op->data[0].intConstantValue == 0) ||
                (right_op->actionType == AST_CONST_FLOAT && dabs(right_op->data[0].floatConstantValue ) < 1e-10) ||
                (left_op->actionType == AST_CONST_INT && left_op->data[0].intConstantValue == 1) ||
                (left_op->actionType == AST_CONST_FLOAT && dabs(1 - left_op->data[0].floatConstantValue) < 1e-10)) {
            (*ast)->right = NULL;
            target = right_op;
        } else if ((left_op->actionType == AST_CONST_INT && left_op->data[0].intConstantValue == 0) ||
                (left_op->actionType == AST_CONST_FLOAT && dabs(left_op->data[0].floatConstantValue) < 1e-10) ||
                (right_op->actionType == AST_CONST_INT && right_op->data[0].intConstantValue == 1) ||
                (right_op->actionType == AST_CONST_FLOAT && dabs(1 - right_op->data[0].floatConstantValue) < 1e-10)) {
            (*ast)->left = NULL;
            target = left_op;
        }
        if (target != NULL) {
            clean_ast(*ast);
            *ast = target;
            *changed = true;
        }
    }
}

void optimise_divide(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    ASTNode *right_op = (*ast)->right;
    if (left_op->actionType == AST_CONST_INT && right_op->actionType == AST_CONST_INT) {
        if (right_op->data[0].intConstantValue == 0) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_DIVISION_BY_ZERO, "Division by zero\n");
            return;
        }
        int64_t new_val = left_op->data[0].intConstantValue / right_op->data[0].intConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_consti(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else if (left_op->actionType == AST_CONST_FLOAT && right_op->actionType == AST_CONST_FLOAT) {
        if (dabs(right_op->data[0].floatConstantValue) < 1e-10) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_DIVISION_BY_ZERO, "Division by zero\n");
            return;
        }
        double new_val = left_op->data[0].floatConstantValue / right_op->data[0].floatConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_constf(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else {
        // If right operand is 1, remove the operation
        if ((right_op->actionType == AST_CONST_INT && right_op->data[0].intConstantValue == 1) ||
                (right_op->actionType == AST_CONST_FLOAT && dabs(1 - right_op->data[0].floatConstantValue) < 1e-10)) {
            (*ast)->left = NULL;
            clean_ast(*ast);
            *ast = left_op;
            *changed = true;
        } else if ((right_op->actionType == AST_CONST_INT && right_op->data[0].intConstantValue == 0) ||
                (right_op->actionType == AST_CONST_FLOAT && dabs(right_op->data[0].floatConstantValue) < 1e-10)) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_DIVISION_BY_ZERO, "Division by zero\n");
            return;
        }
    }
}

void optimise_negate(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    if (left_op->actionType == AST_CONST_INT) {
        (*ast)->left = NULL;
        clean_ast(*ast);
        (*ast) = left_op;
        left_op->data[0].intConstantValue = -left_op->data[0].intConstantValue;
        *changed = true;
    } else if (left_op->actionType == AST_CONST_FLOAT) {
        (*ast)->left = NULL;
        clean_ast(*ast);
        (*ast) = left_op;
        left_op->data[0].floatConstantValue = -left_op->data[0].floatConstantValue;
        *changed = true;
    } else if (left_op->actionType == AST_AR_NEGATE) {
        // Two - operators cancel out
        ASTNode *repl = left_op->left;
        (*ast)->left->left = NULL;
        clean_ast(*ast);
        (*ast) = repl;
        *changed = true;
    }
}

void optimise_ast(ASTNode **ast, bool *changed) {
    // Traverse the AST using in-order traversal
    if (*ast == NULL) {
        return;
    }
    if ((*ast)->actionType == AST_LIST) {
        // Walk through all elements of the list
        for (unsigned i = 0; i < (*ast)->dataCount; i++) {
            optimise_ast(&(*ast)->data[i].astPtr, changed);
        }
    } else {
        optimise_ast(&(*ast)->left, changed);
    }
    switch ((*ast)->actionType) {
        case AST_ADD:
            optimise_add(ast, changed);
            break;
        case AST_SUBTRACT:
            optimise_subtract(ast, changed);
            break;
        case AST_MULTIPLY:
            optimise_multiply(ast, changed);
            break;
        case AST_DIVIDE:
            optimise_divide(ast, changed);
            break;
        case AST_AR_NEGATE:
            optimise_negate(ast, changed);
            break;
        default:
            break;
    }
    if ((*ast)->actionType != AST_LIST) {
        optimise_ast(&(*ast)->right, changed);
    }
}

void optimise_expressions(CFStatement *stat, bool *changed) {
    if (stat != NULL && !is_statement_empty(stat)) {
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

    if (stat != NULL && stat->followingStatement != NULL) {
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
    bool changed = true;
    while (compiler_result == COMPILER_RESULT_SUCCESS && changed) {
        changed = false;
        fold_constants(&changed);
    }
}