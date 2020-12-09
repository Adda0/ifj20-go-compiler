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
#include "variable_vector.h"


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
        // If one of the operands is 0, the operation is also constant (doesn't apply to float).
        // If one of the operands is 1, we can remove the multiplication.
        ASTNode *target = NULL;
        if ((right_op->actionType == AST_CONST_INT && right_op->data[0].intConstantValue == 0) ||
                (left_op->actionType == AST_CONST_INT && left_op->data[0].intConstantValue == 1) ||
                (left_op->actionType == AST_CONST_FLOAT && dabs(1 - left_op->data[0].floatConstantValue) < 1e-10)) {
            (*ast)->right = NULL;
            target = right_op;
        } else if ((left_op->actionType == AST_CONST_INT && left_op->data[0].intConstantValue == 0) ||
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

void optimise_log_neg(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    if (left_op->actionType == AST_CONST_BOOL) {
        (*ast)->left = NULL;
        clean_ast(*ast);
        (*ast) = left_op;
        left_op->data[0].boolConstantValue = !left_op->data[0].boolConstantValue;
        *changed = true;
    } else if (left_op->actionType == AST_LOG_NOT) {
        // two ! operators cancel out
        ASTNode *repl = left_op->left;
        (*ast)->left->left = NULL;
        clean_ast(*ast);
        (*ast) = repl;
        *changed = true;
    }
}

void optimise_log_and(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    ASTNode *right_op = (*ast)->right;
    if (left_op->actionType == AST_CONST_BOOL && right_op->actionType == AST_CONST_BOOL) {
        bool new_val = left_op->data[0].boolConstantValue && right_op->data[0].boolConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_constb(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else if (left_op->actionType == AST_CONST_BOOL && !left_op->data[0].boolConstantValue) {
        // Short circuit optimization, the result will be false
        (*ast)->left = NULL;
        clean_ast(*ast);
        (*ast) = left_op;
        *changed = true;
    }
}

void optimise_log_or(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    ASTNode *right_op = (*ast)->right;
    if (left_op->actionType == AST_CONST_BOOL && right_op->actionType == AST_CONST_BOOL) {
        bool new_val = left_op->data[0].boolConstantValue || right_op->data[0].boolConstantValue;
        clean_ast(*ast);
        *ast = ast_leaf_constb(new_val);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    } else if (left_op->actionType == AST_CONST_BOOL && left_op->data[0].boolConstantValue) {
        // Short circuit optimization, the result will be true
        (*ast)->left = NULL;
        clean_ast(*ast);
        (*ast) = left_op;
        *changed = true;
    }
}

bool compare_ints(ASTNode **ast) {
    int64_t val1 = (*ast)->left->data[0].intConstantValue;
    int64_t val2 = (*ast)->right->data[0].intConstantValue;
    switch ((*ast)->actionType) {
        case AST_LOG_EQ:
            return val1 == val2;
        case AST_LOG_NEQ:
            return val1 != val2;
        case AST_LOG_GT:
            return val1 > val2;
        case AST_LOG_LT:
            return val1 < val2;
        case AST_LOG_GTE:
            return val1 >= val2;
        case AST_LOG_LTE:
            return val1 <= val2;
        default:
            return false;
    }
}

bool compare_floats(ASTNode **ast) {
    double val1 = (*ast)->left->data[0].floatConstantValue;
    double val2 = (*ast)->right->data[0].floatConstantValue;
    switch ((*ast)->actionType) {
        case AST_LOG_EQ:
            return val1 == val2;
        case AST_LOG_NEQ:
            return val1 != val2;
        case AST_LOG_GT:
            return val1 > val2;
        case AST_LOG_LT:
            return val1 < val2;
        case AST_LOG_GTE:
            return val1 >= val2;
        case AST_LOG_LTE:
            return val1 <= val2;
        default:
            return false;
    }
}

bool compare_strings(ASTNode **ast) {
    const char *val1 = (*ast)->left->data[0].stringConstantValue;
    const char *val2 = (*ast)->right->data[0].stringConstantValue;
    int res = strcmp(val1, val2);
    switch ((*ast)->actionType) {
        case AST_LOG_EQ:
            return res == 0;
        case AST_LOG_NEQ:
            return res !=0;
        case AST_LOG_GT:
            return res > 0;
        case AST_LOG_LT:
            return res < 0;
        case AST_LOG_GTE:
            return res >= 0;
        case AST_LOG_LTE:
            return res <= 0;
        default:
            return false;
    }
}
void optimise_relational_operator(ASTNode **ast, bool *changed) {
    ASTNode *left_op = (*ast)->left;
    ASTNode *right_op = (*ast)->right;
    bool result;
    bool modify = false;
    if (left_op->actionType == AST_CONST_INT && right_op->actionType == AST_CONST_INT) {
        result = compare_ints(ast);
        modify = true;
    } else if (left_op->actionType == AST_CONST_FLOAT && right_op->actionType == AST_CONST_FLOAT) {
        result = compare_floats(ast);
        modify = true;
    } else if (left_op->actionType == AST_CONST_STRING && right_op->actionType == AST_CONST_STRING) {
        result = compare_strings(ast);
        modify = true;
    } else if (left_op->actionType == AST_CONST_BOOL && right_op->actionType == AST_CONST_BOOL) {
        bool val1 = left_op->data[0].boolConstantValue;
        bool val2 = right_op->data[0].boolConstantValue;
        switch ((*ast)->actionType) {
            case AST_LOG_EQ:
                result = val1 == val2;
                break;
            case AST_LOG_NEQ:
                result = val1 != val2;
                break;
            default:
                return;
        }
        modify = true;
    }
    if (modify) {
        // Create new bool node
        clean_ast(*ast);
        *ast = ast_leaf_constb(result);
        if (*ast == NULL) {
            stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            return;
        }
        *changed = true;
    }
}

void optimise_ast(ASTNode **ast, bool *changed) {
    // Traverse the AST using post-order traversal
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
        optimise_ast(&(*ast)->right, changed);
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
        case AST_LOG_NOT:
            optimise_log_neg(ast, changed);
            break;
        case AST_LOG_AND:
            optimise_log_and(ast, changed);
            break;
        case AST_LOG_OR:
            optimise_log_or(ast, changed);
            break;
        case AST_LOG_EQ:
        case AST_LOG_NEQ:
        case AST_LOG_LT:
        case AST_LOG_GT:
        case AST_LOG_LTE:
        case AST_LOG_GTE:
            optimise_relational_operator(ast, changed);
            break;
        default:
            break;
    }
}

void optimise_expressions(CFStatement *stat, bool *changed) {
    if (stat != NULL && !is_statement_empty(stat)) {
        switch (stat->statementType) {
            case CF_BASIC:
            case CF_RETURN:
                if (stat->data.bodyAst != NULL && !ast_infer_node_type(stat->data.bodyAst)) return;
                optimise_ast(&stat->data.bodyAst, changed);
                break;
            case CF_IF:
                if (!ast_infer_node_type(stat->data.ifData->conditionalAst)) return;
                optimise_ast(&stat->data.ifData->conditionalAst, changed);
                optimise_expressions(stat->data.ifData->thenStatement, changed);
                optimise_expressions(stat->data.ifData->elseStatement, changed);
                break;
            case CF_FOR:
                if (stat->data.forData->definitionAst != NULL &&
                        !ast_infer_node_type(stat->data.forData->definitionAst)) return;
                optimise_ast(&stat->data.forData->definitionAst, changed);
                if (!ast_infer_node_type(stat->data.forData->conditionalAst)) return;
                optimise_ast(&stat->data.forData->conditionalAst, changed);
                if (stat->data.forData->afterthoughtAst != NULL &&
                    !ast_infer_node_type(stat->data.forData->afterthoughtAst)) return;
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

void propagate_into_expression(ASTNode **ast, bool *changed, VariableVector *vector, STSymbol *unary_symb) {
    if (*ast == NULL) {
        return;
    }
    if ((*ast)->actionType == AST_LIST) {
        // Walk through all elements of the list
        for (unsigned i = 0; i < (*ast)->dataCount; i++) {
            propagate_into_expression(&(*ast)->data[i].astPtr, changed, vector, unary_symb);
        }
    } else {
        if ((*ast)->actionType != AST_FUNC_CALL) {
            propagate_into_expression(&(*ast)->left, changed, vector, unary_symb);
        }
        propagate_into_expression(&(*ast)->right, changed, vector, unary_symb);
    }

    if ((*ast)->actionType == AST_ID) {
        Variable *found = vv_find(vector, (*ast)->data[0].symbolTableItemPtr);
        if (found != NULL) {
            // Replace ID with a constant
            ASTNode *tmp = *ast;
            switch(found->data.type) {
                case AST_CONST_BOOL:
                    *ast = ast_leaf_constb(found->data.data.boolConstantValue);
                    break;
                case AST_CONST_INT:
                    *ast = ast_leaf_consti(found->data.data.intConstantValue);
                    break;
                case AST_CONST_FLOAT:
                    *ast = ast_leaf_constf(found->data.data.floatConstantValue);
                    break;
                case AST_CONST_STRING:
                    *ast = ast_leaf_consts(found->data.data.stringConstantValue);
                    break;
                default:
                    return;
            }
            if (*ast == NULL) {
                stderr_message("optimiser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Out of memory\n");
            }
            *changed = true;
            if (found->data.symbol == unary_symb) {
                unary_symb->reference_counter++;
            }
            clean_ast(tmp);
        }
    }
}

void propagate_ast_constants(ASTNode **ast, bool remove_only, bool add_new, bool *changed, VariableVector *vector) {
    // First try to propagate in the expression. For assignments and definitions
    // propagate only on the right side. This covers cases such as
    // a := 5
    // b := a
    if (*ast == NULL) {
        return;
    }
    STSymbol *symb = NULL;
    if (!remove_only) {
        switch ((*ast)->actionType) {
            case AST_DEFINE:
            case AST_ASSIGN:
                // A bit of a hack, using unary assign operator (e.g. +=) can be determined based on actionType.
                // Normal assignments have AST_LIST on the left, whereas unary assignments have only AST_ID (only
                // one thing can be assigned at a time). We want to avoid reducing reference counter when cleaning
                // the AST as the UNARY assignment doesn't count as a reference
                if ((*ast)->left->actionType == AST_ID) {
                    symb = (*ast)->left->data[0].symbolTableItemPtr;
                }
                propagate_into_expression(&(*ast)->right, changed, vector, symb);
                break;
            default:
                propagate_into_expression(ast, changed, vector, NULL);
                break;
        }
    }

    // Now introduce new constants and remove what is no longer constant
    if ((*ast)->actionType == AST_DEFINE) {
        // Check if a constant isn't assigned in the definition
        if ((*ast)->left->dataCount == (*ast)->right->dataCount) {
            for (unsigned i = 0; i < (*ast)->left->dataCount; i++) {
                bool new_constant = false;
                VariableData data;
                ASTNode *current = (*ast)->right->data[i].astPtr;
                STSymbol *symbol = (*ast)->left->data[i].astPtr->data[0].symbolTableItemPtr;
                switch (current->actionType) {
                    case AST_CONST_FLOAT:
                        new_constant = true;
                        data.data.floatConstantValue = current->data[0].floatConstantValue;
                        data.type = AST_CONST_FLOAT;
                        break;
                    case AST_CONST_INT:
                        new_constant = true;
                        data.data.intConstantValue = current->data[0].intConstantValue;
                        data.type = AST_CONST_INT;
                        break;
                    case AST_CONST_STRING:
                        new_constant = true;
                        data.data.stringConstantValue = current->data[0].stringConstantValue;
                        data.type = AST_CONST_STRING;
                        break;
                    case AST_CONST_BOOL:
                        new_constant = true;
                        data.data.boolConstantValue = current->data[0].boolConstantValue;
                        data.type = AST_CONST_BOOL;
                        break;
                    default:
                        // This could be a redefinition of a symbol with a non-constant, remove it
                        vv_remove_symbol(vector, symbol);
                        break;
                }
                if (new_constant && !remove_only && add_new) {
                    data.symbol = symbol;
                    vv_append(vector, data);
                }
            }
        } else {
            // Function call, we can't deduce constants, just remove all variables on LHS from vector
            for (unsigned i = 0; i < (*ast)->left->dataCount; i++) {
                vv_remove_symbol(vector, (*ast)->left->data[i].astPtr->data[0].symbolTableItemPtr);
            }
        }
    } else if ((*ast)->actionType == AST_ASSIGN) {
        // Assignment, invalidate all assigned variables as constants
        if ((*ast)->left->actionType == AST_ID) {
            vv_remove_symbol(vector, (*ast)->left->data[0].symbolTableItemPtr);
        } else {
            for (unsigned i = 0; i < (*ast)->left->dataCount; i++) {
                vv_remove_symbol(vector, (*ast)->left->data[i].astPtr->data[0].symbolTableItemPtr);
            }
        }
    }
}

void propagate_function_constants(CFStatement *stat, bool remove, bool add, bool *changed, VariableVector *vector) {
    // Handle blocks nested in for, we can't add new constants inside a for block.
    if (stat != NULL && !is_statement_empty(stat)) {
        switch(stat->statementType) {
            case CF_BASIC:
            case CF_RETURN:
                if (stat->data.bodyAst != NULL && !ast_infer_node_type(stat->data.bodyAst)) return;
                propagate_ast_constants(&stat->data.bodyAst, remove, add, changed, vector);
                break;
            case CF_IF:
                if (!ast_infer_node_type(stat->data.ifData->conditionalAst)) return;
                propagate_ast_constants(&stat->data.ifData->conditionalAst, remove, add, changed, vector);
                propagate_function_constants(stat->data.ifData->thenStatement, remove, add, changed, vector);
                propagate_function_constants(stat->data.ifData->elseStatement, remove, add, changed, vector);
                break;
            case CF_FOR:
                // First invalidate all variables created in for definition, afterthought and its body
                if (!ast_infer_node_type(stat->data.forData->definitionAst)) return;
                propagate_ast_constants(&stat->data.forData->definitionAst, true, false, changed, vector);
                if (!ast_infer_node_type(stat->data.forData->conditionalAst)) return;
                propagate_ast_constants(&stat->data.forData->afterthoughtAst, true, false, changed, vector);
                propagate_function_constants(stat->data.forData->bodyStatement, true, false, changed, vector);
                // Now propagate whatever is constant after processing the whole loop
                propagate_ast_constants(&stat->data.forData->conditionalAst, false, true, changed, vector);
                propagate_function_constants(stat->data.forData->bodyStatement, false, false, changed, vector);
                break;
        }
    }

    if (stat != NULL && stat->followingStatement != NULL) {
        propagate_function_constants(stat->followingStatement, remove, add, changed, vector);
    }
}

void propagate_constants(bool *changed) {
    CFProgram *prog = get_program();
    CFFuncListNode *n = prog->functionList;
    while (n != NULL) {
        VariableVector vector;
        vv_init(&vector);
        propagate_function_constants(n->fun.rootStatement, false, true, changed, &vector);
        n = n->next;
        vv_free(&vector);
    }
}

void rebind_adjacent_statements(CFStatement *stat, CFFunction *fun) {
    if (stat == fun->rootStatement) {
        fun->rootStatement = stat->followingStatement;
        if (stat->followingStatement != NULL) {
            stat->followingStatement->parentStatement = NULL;
        }
    } else {
        stat->parentStatement->followingStatement = stat->followingStatement;
        if (stat->followingStatement != NULL) {
            stat->followingStatement->parentStatement = stat->parentStatement;
        }
    }
}

void remove_function_dead_code(CFStatement *stat, CFFunction *fun) {
    SymbolTable *table;
    SymbolTable *parent_table;
    if (stat != NULL && !is_statement_empty(stat)) {
        switch (stat->statementType) {
            case CF_IF:
                if (stat->data.ifData->conditionalAst->actionType == AST_CONST_BOOL &&
                        !stat->data.ifData->conditionalAst->data[0].boolConstantValue) {
                    // If false, remove the block
                    if (stat->data.ifData->elseStatement == NULL) {
                        // If without else, completely remove the statement
                        rebind_adjacent_statements(stat, fun);
                        stat->followingStatement = NULL;
                        CFStatement *tmp = stat;
                        stat = stat->parentStatement;
                        clean_stat(tmp, tmp->localSymbolTable);
                    } else {
                        // Has else, convert else into if true
                        table = stat->data.ifData->thenStatement->localSymbolTable;
                        parent_table = stat->data.ifData->thenStatement->parentStatement->localSymbolTable;
                        stat->data.ifData->conditionalAst->data[0].boolConstantValue = true;
                        clean_stat(stat->data.ifData->thenStatement, stat->localSymbolTable);
                        stat->data.ifData->thenStatement = stat->data.ifData->elseStatement;
                        stat->data.ifData->elseStatement = NULL;
                        if (table != parent_table) {
                            symtable_free(table);
                        }
                        remove_function_dead_code(stat->data.ifData->thenStatement, fun);
                    }
                } else if (stat->data.ifData->conditionalAst->actionType == AST_CONST_BOOL &&
                        stat->data.ifData->conditionalAst->data[0].boolConstantValue){
                    // If true, remove useless else
                    if (stat->data.ifData->elseStatement != NULL) {
                        table = stat->data.ifData->elseStatement->localSymbolTable;
                        parent_table = stat->data.ifData->elseStatement->parentStatement->localSymbolTable;
                        clean_stat(stat->data.ifData->elseStatement, stat->localSymbolTable);
                        stat->data.ifData->elseStatement = NULL;
                        if (table != parent_table) {
                            symtable_free(table);
                        }
                    }
                    remove_function_dead_code(stat->data.ifData->thenStatement, fun);
                } else {
                    remove_function_dead_code(stat->data.ifData->thenStatement, fun);
                    remove_function_dead_code(stat->data.ifData->elseStatement, fun);
                }
                break;
            case CF_FOR:
                if (stat->data.forData->conditionalAst->actionType == AST_CONST_BOOL &&
                        !stat->data.forData->conditionalAst->data[0].boolConstantValue) {
                    // Discard dead for loop
                    rebind_adjacent_statements(stat, fun);
                    // Move one step back so that stat->followingStatement moves correctly forward
                    stat->followingStatement = NULL;
                    CFStatement *tmp = stat;
                    stat = stat->parentStatement;
                    clean_stat(tmp, tmp->localSymbolTable);
                } else {
                    remove_function_dead_code(stat->data.forData->bodyStatement, fun);
                }
                break;
            default:
                break;
        }
    }
    if (stat != NULL && stat->followingStatement != NULL) {
        remove_function_dead_code(stat->followingStatement, fun);
    }
}

void remove_dead_code() {
    CFProgram *prog = get_program();
    CFFuncListNode *n = prog->functionList;
    while (n != NULL) {
        remove_function_dead_code(n->fun.rootStatement, &n->fun);
        n = n->next;
    }
}

void optimiser_optimise() {
    bool changed = true;
    while (compiler_result == COMPILER_RESULT_SUCCESS && changed) {
        changed = false;
        fold_constants(&changed);
        propagate_constants(&changed);
    }
    remove_dead_code();
}
