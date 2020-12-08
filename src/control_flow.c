/** @file control_flow.c
 *
 * IFJ20 compiler
 *
 * @brief Implements the control flow graph.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#include "control_flow.h"
#include <stdlib.h>
#include <string.h>

#define CF_ALLOC_CHECK(ptr) do { if ((ptr) == NULL) { cf_error = CF_ERROR_INTERNAL; return; } } while(0)
#define CF_ALLOC_CHECK_RN(ptr) do { if ((ptr) == NULL) { cf_error = CF_ERROR_INTERNAL; return NULL; } } while(0)
#define CF_ACT_FUN_CHECK() do { if (activeFunc == NULL) { cf_error = CF_ERROR_NO_ACTIVE_FUNCTION; return; } } while(0)
#define CF_ACT_FUN_CHECK_RN() do { if (activeFunc == NULL) { cf_error = CF_ERROR_NO_ACTIVE_FUNCTION; return NULL; } } while(0)
#define CF_ACT_STAT_CHECK() do { if (activeStat == NULL) { cf_error = CF_ERROR_NO_ACTIVE_STATEMENT; return; } } while(0)
#define CF_ACT_STAT_CHECK_RN() do { if (activeStat == NULL) { cf_error = CF_ERROR_NO_ACTIVE_STATEMENT; return NULL; } } while(0)
#define CF_ACT_AST_CHECK() do { if (activeAst == NULL) { cf_error = CF_ERROR_NO_ACTIVE_AST; return; } } while(0)
#define CF_ACT_AST_CHECK_RN() do { if (activeAst == NULL) { cf_error = CF_ERROR_NO_ACTIVE_AST; return NULL; } } while(0)

extern ASTNode *cf_ast_init(ASTNewNodeTarget target, ASTNodeType type); // NOLINT(readability-redundant-declaration)
extern ASTNode *cf_ast_init_for_list(ASTNodeType type, int listDataIndex); // NOLINT(readability-redundant-declaration)

static CFProgram *program;
CFError cf_error = CF_NO_ERROR;

CFStatement *activeStat;
CFFunction *activeFunc;
ASTNode *activeAst;

CFProgram *get_program() {
    return program;
}

void cf_init() {
    program = calloc(1, sizeof(struct cfgraph_program_structure));
    CF_ALLOC_CHECK(program);
}

void cf_assign_global_symtable(SymbolTable *symbolTable) {
    if (program->globalSymtable != NULL) {
        cf_error = CF_ERROR_SYMTABLE_ALREADY_ASSIGNED;
        return;
    }

    program->globalSymtable = symbolTable;
}

CFFunction *cf_get_function(const char *name, bool setActive) {
    CFFuncListNode *n = program->functionList;
    while (n != NULL) {
        if (strcmp(n->fun.name, name) == 0) {
            return &n->fun;
        }
        n = n->next;
    }

    return NULL;
}

CFFunction *cf_make_function(const char *name) {
    CFFuncListNode *n = program->functionList;
    CFFuncListNode *newNode = calloc(1, sizeof(CFFuncListNode));
    CF_ALLOC_CHECK_RN(newNode);

    program->functionList = newNode;
    if (n != NULL) n->previous = newNode;

    newNode->next = n;
    newNode->previous = NULL;
    CFFunction *newFunctionNode = &newNode->fun;

    newFunctionNode->name = malloc(strlen(name) + 1);
    CF_ALLOC_CHECK_RN(newFunctionNode->name);
    strcpy((char *) newFunctionNode->name, name);

    if (strcmp(name, "main") == 0) {
        if (program->mainFunc == NULL) {
            program->mainFunc = newFunctionNode;
        } else {
            cf_error = CF_ERROR_MAIN_REDEFINITION;
            return NULL;
        }
    }

    activeStat = NULL;
    activeFunc = newFunctionNode;

    return newFunctionNode;
}

void cf_add_argument(const char *name, CFDataType type) {
    CF_ACT_FUN_CHECK();
    if (program->mainFunc == activeFunc) {
        cf_error = CF_ERROR_MAIN_NO_ARGUMENTS_OR_RETURN_VALUES;
        return;
    }

    CFVarListNode *n = activeFunc->arguments;
    CFVarListNode *newNode = malloc(sizeof(CFVarListNode));
    CF_ALLOC_CHECK(newNode);

    activeFunc->arguments = newNode;
    if (n != NULL) n->previous = newNode;
    newNode->next = n;
    newNode->previous = NULL;

    newNode->variable.name = malloc(strlen(name) + 1);
    strcpy((char *) newNode->variable.name, name);

    newNode->variable.dataType = type;
    newNode->variable.position = activeFunc->argumentsCount;
    activeFunc->argumentsCount++;
}

void cf_add_return_value(const char *name, CFDataType type) {
    CF_ACT_FUN_CHECK();
    if (program->mainFunc == activeFunc) {
        cf_error = CF_ERROR_MAIN_NO_ARGUMENTS_OR_RETURN_VALUES;
        return;
    }

    CFVarListNode *n = activeFunc->returnValues;

    if (n != NULL) {
        if (n->variable.name != NULL && name == NULL || n->variable.name == NULL && name != NULL) {
            cf_error = CF_ERROR_RETURN_VALUES_NAMING_MISMATCH;
            return;
        }
    }

    CFVarListNode *newNode = malloc(sizeof(CFVarListNode));
    CF_ALLOC_CHECK(newNode);

    activeFunc->returnValues = newNode;
    if (n != NULL) n->previous = newNode;
    newNode->next = n;
    newNode->previous = NULL;

    if (name == NULL) {
        newNode->variable.name = NULL;
    } else {
        newNode->variable.name = malloc(strlen(name) + 1);
        strcpy((char *) newNode->variable.name, name);
    }

    newNode->variable.dataType = type;
    newNode->variable.position = activeFunc->returnValuesCount;
    activeFunc->returnValuesCount++;
}

CFStatement *cf_make_next_statement(CFStatementType statementType) {
    CF_ACT_FUN_CHECK_RN();

    CFStatement *newStat = calloc(1, sizeof(CFStatement));
    CF_ALLOC_CHECK_RN(newStat);

    newStat->parentFunction = activeFunc;
    newStat->parentStatement = activeStat;
    newStat->statementType = statementType;

    if (activeStat != NULL) {
        if (activeStat->statementType == CF_FOR) {
            // Find the first non-for parent statement to acquire the correct symtable
            CFStatement *bestParent = activeStat->parentStatement;

            while (bestParent != NULL && bestParent->statementType == CF_FOR) {
                bestParent = bestParent->parentStatement;
            }

            if (bestParent == NULL) {
                newStat->localSymbolTable = activeFunc->symbolTable;
            } else {
                newStat->localSymbolTable = bestParent->localSymbolTable;
            }
        } else if (activeStat->localSymbolTable != NULL) {
            newStat->localSymbolTable = activeStat->localSymbolTable;
        }

        if (activeStat->statementType == CF_IF && activeStat->data.ifData->elseStatement == NULL) {
            // If this statement is an IF with no else statement, increase its popCount by one
            // to ensure it cannot be popped into anymore
            activeStat->popCount++;
        }
    } else {
        newStat->localSymbolTable = activeFunc->symbolTable;
    }

    if (activeFunc->rootStatement == NULL) {
        activeFunc->rootStatement = newStat;
        newStat->parentBranchStatement = newStat;
    }

    if (activeStat != NULL) {
        activeStat->followingStatement = newStat;
        newStat->parentBranchStatement = activeStat->parentBranchStatement;
    }

    switch (statementType) {
        case CF_BASIC:
        case CF_RETURN:
            break;
        case CF_IF:
            newStat->data.ifData = calloc(1, sizeof(CFStatementIf));
            break;
        case CF_FOR:
            newStat->data.forData = calloc(1, sizeof(CFStatementFor));
            break;
        default:
            cf_error = CF_ERROR_INVALID_ENUM_VALUE;
            return NULL;
    }

    activeStat = newStat;
    return newStat;
}

void cf_assign_function_symtable(SymbolTable *symbolTable) {
    CF_ACT_FUN_CHECK();
    if (activeFunc->rootStatement != NULL) {
        cf_error = CF_ERROR_SYMTABLE_TARGET_HAS_CHILDREN;
        return;
    }

    activeFunc->symbolTable = symbolTable;
}

void cf_assign_statement_symtable(SymbolTable *symbolTable) {
    CF_ACT_STAT_CHECK();
    if (activeStat->followingStatement != NULL) {
        cf_error = CF_ERROR_SYMTABLE_TARGET_HAS_CHILDREN;
        return;
    }

    activeStat->localSymbolTable = symbolTable;
}

void cf_use_ast(CFASTTarget target) {
    CF_ACT_AST_CHECK();
    cf_use_ast_explicit(activeAst, target);
}

void cf_use_ast_explicit(ASTNode *ast, CFASTTarget target) {
    if (ast == NULL) {
        return;
    }

    // TODO: this is a bit spaghetti, a cleanup would be nice
    switch (activeStat->statementType) {
        case CF_BASIC:
            if (target != CF_STATEMENT_BODY) {
                cf_error = CF_ERROR_INVALID_AST_TARGET;
                return;
            }

            if (ast->actionType != AST_DEFINE
                && ast->actionType != AST_ASSIGN
                && ast->actionType != AST_FUNC_CALL) {
                cf_error = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.bodyAst = ast;
            break;
        case CF_IF:
            if (target != CF_IF_CONDITIONAL) {
                cf_error = CF_ERROR_INVALID_AST_TARGET;
                return;
            }

            if (ast->actionType != AST_FUNC_CALL
                && ast->actionType != AST_ID
                && ast->actionType != AST_CONST_BOOL
                && !(ast->actionType >= AST_LOGIC && ast->actionType < AST_CONTROL)) {
                cf_error = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.ifData->conditionalAst = ast;
            break;
        case CF_FOR:
            switch (target) {
                case CF_FOR_DEFINITION:
                    if (ast->actionType != AST_DEFINE) {
                        cf_error = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->definitionAst = ast;
                    break;
                case CF_FOR_CONDITIONAL:
                    if (ast->actionType != AST_FUNC_CALL
                        && ast->actionType != AST_ID
                        && ast->actionType != AST_CONST_BOOL
                        && !(ast->actionType >= AST_LOGIC && ast->actionType < AST_CONTROL)) {
                        cf_error = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->conditionalAst = ast;
                    break;
                case CF_FOR_AFTERTHOUGHT:
                    if (ast->actionType != AST_ASSIGN) {
                        cf_error = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->afterthoughtAst = ast;
                    break;
                default:
                    cf_error = CF_ERROR_INVALID_AST_TARGET;
                    return;
            }
            break;
        case CF_RETURN:
            if (target != CF_RETURN_LIST) {
                cf_error = CF_ERROR_INVALID_AST_TARGET;
                return;
            }

            if (ast->actionType != AST_LIST) {
                cf_error = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.bodyAst = ast;
            break;
    }
}

CFStatement *cf_pop_previous_branched_statement() {
    if (activeStat == NULL) return NULL;

    CFStatement *n = activeStat->parentStatement;
    while (!(n->statementType == CF_IF && n->popCount < 2) && !(n->statementType == CF_FOR && n->popCount < 1)) {
        n = n->parentStatement;

        if (n == NULL) {
            return NULL;
        }
    }

    activeStat = n;
    n->popCount++;
    return n;
}

CFStatement *cf_make_if_then_statement(CFStatementType type) {
    if (activeStat->statementType != CF_IF) {
        cf_error = CF_ERROR_INVALID_OPERATION;
        return NULL;
    }

    // Save the currently active statement
    CFStatement *currentActive = activeStat;
    CFStatement *currentFollowing = activeStat->followingStatement;

    // Make a new statement which will be set as active
    CFStatement *newStat = cf_make_next_statement(type);
    if (newStat == NULL) return NULL; // The error has been handled
    currentActive->popCount--;

    newStat->parentBranchStatement = currentActive;

    // Restore the previously active statement's follower and set the newly created one as thenStatement instead
    currentActive->followingStatement = currentFollowing;
    currentActive->data.ifData->thenStatement = newStat;

    return newStat;
}

CFStatement *cf_make_if_else_statement(CFStatementType type) {
    if (activeStat->statementType != CF_IF) {
        cf_error = CF_ERROR_INVALID_OPERATION;
        return NULL;
    }

    // Save the currently active statement
    CFStatement *currentActive = activeStat;
    CFStatement *currentFollowing = activeStat->followingStatement;

    // Make a new statement which will be set as active
    CFStatement *newStat = cf_make_next_statement(type);
    if (newStat == NULL) return NULL; // The error has been handled
    currentActive->popCount--;

    newStat->parentBranchStatement = currentActive;

    // Restore the previously active statement's follower and set the newly created one as thenStatement instead
    currentActive->followingStatement = currentFollowing;
    currentActive->data.ifData->elseStatement = newStat;

    return newStat;
}

CFStatement *cf_make_for_body_statement(CFStatementType type) {
    if (activeStat->statementType != CF_FOR) {
        cf_error = CF_ERROR_INVALID_OPERATION;
        return NULL;
    }

    // Save the currently active statement
    CFStatement *currentActive = activeStat;
    CFStatement *currentFollowing = activeStat->followingStatement;

    // Make a new statement which will be set as active
    CFStatement *newStat = cf_make_next_statement(type);
    if (newStat == NULL) return NULL; // The error has been handled

    newStat->parentBranchStatement = currentActive;

    // Restore the previously active statement's follower and set the newly created one as thenStatement instead
    currentActive->followingStatement = currentFollowing;
    currentActive->data.forData->bodyStatement = newStat;

    return newStat;
}

bool is_statement_empty(CFStatement *stat) {
    if (stat == NULL) return true;

    switch (stat->statementType) {
        case CF_BASIC:
            if (stat->parentStatement != NULL && (stat->parentStatement->statementType == CF_IF
                                                  || stat->parentStatement->statementType == CF_FOR)) {
                return false;
            }
            return is_ast_empty(stat->data.bodyAst);
        case CF_IF:
            return is_ast_empty(stat->data.ifData->conditionalAst) ||
                   (is_statement_empty(stat->data.ifData->thenStatement) &&
                    is_statement_empty(stat->data.ifData->elseStatement));
        case CF_FOR:
            return is_statement_empty(stat->data.forData->bodyStatement);
        case CF_RETURN:
            for (unsigned i = 0; i < stat->data.bodyAst->dataCount; i++) {
                if (is_ast_empty(stat->data.bodyAst->data[i].astPtr)) return true;
            }

            return false;
        default:
            return true;
    }
}

void clean_stat(CFStatement *stat) {
    if (stat == NULL) return;
    switch (stat->statementType) {
        case CF_BASIC:
        case CF_RETURN:
            clean_ast(stat->data.bodyAst);
            break;
        case CF_IF:
            if (stat->data.ifData == NULL) break;

            clean_ast(stat->data.ifData->conditionalAst);

            if (stat->data.ifData->thenStatement != NULL) {
                SymbolTable *table = stat->data.ifData->thenStatement->localSymbolTable;
                clean_stat(stat->data.ifData->thenStatement);
                symtable_free(table);
            }

            if (stat->data.ifData->elseStatement != NULL) {
                if (stat->data.ifData->elseStatement->statementType == CF_IF) {
                    clean_stat(stat->data.ifData->elseStatement);
                } else {
                    SymbolTable *table = stat->data.ifData->elseStatement->localSymbolTable;
                    clean_stat(stat->data.ifData->elseStatement);
                    symtable_free(table);
                }
            }

            free(stat->data.ifData);
            break;
        case CF_FOR:
            if (stat->data.forData == NULL) break;

            clean_ast(stat->data.forData->conditionalAst);
            clean_ast(stat->data.forData->definitionAst);
            clean_ast(stat->data.forData->afterthoughtAst);
            SymbolTable *header_table = stat->localSymbolTable;

            if (stat->data.forData->bodyStatement != NULL) {
                SymbolTable *table = stat->data.forData->bodyStatement->localSymbolTable;
                clean_stat(stat->data.forData->bodyStatement);
                symtable_free(table);
            }

            symtable_free(header_table);
            free(stat->data.forData);
            break;
    }

    clean_stat(stat->followingStatement);
    free(stat);
}

static void clean_varlist(CFVarListNode *begin) {
    while (begin != NULL) {
        CFVarListNode *next = begin->next;
        free((void *) begin->variable.name);
        free(begin);
        begin = next;
    }
}

void cf_clean_all() {
    if (program == NULL) return;

    CFFuncListNode *n = program->functionList;

    while (n != NULL) {
        clean_stat(n->fun.rootStatement);
        if (n->fun.symbolTable != NULL) {
            symtable_free(n->fun.symbolTable);
        }
        clean_varlist(n->fun.arguments);
        clean_varlist(n->fun.returnValues);
        CFFuncListNode *toFree = n;
        n = n->next;
        free((void *) toFree->fun.name);
        free(toFree);
    }

    if (program->globalSymtable != NULL) {
        symtable_free(program->globalSymtable);
    }
    free(program);
}

// ---- Deprecated functions ----
ASTNode *cf_ast_init_with_data(ASTNewNodeTarget target, ASTNodeType type, unsigned dataCount) {
    if (target != AST_ROOT) {
        CF_ACT_AST_CHECK_RN();
    }

    ASTNode *newNode = calloc(1, sizeof(ASTNode) + dataCount * sizeof(ASTNodeData));
    CF_ALLOC_CHECK_RN(newNode);

    if (target != AST_ROOT) {
        newNode->parent = activeAst;
        if (target == AST_LEFT_OPERAND || target == AST_UNARY_OPERAND) {
            activeAst->left = newNode;
        } else {
            activeAst->right = newNode;
        }
    }

    newNode->actionType = type;
    newNode->dataCount = dataCount;

    activeAst = newNode;
    return newNode;
}

ASTNode *cf_ast_init_for_list_with_data(ASTNodeType type, unsigned dataCount, int listDataIndex) {
    CF_ACT_AST_CHECK_RN();
    if (activeAst->actionType != AST_LIST) {
        cf_error = CF_ERROR_INVALID_AST_TYPE;
        return NULL;
    }

    if (listDataIndex == -1) {
        listDataIndex = activeAst->dataPointerIndex++; // NOLINT(cppcoreguidelines-narrowing-conversions)
    }

    ASTNode *newNode = calloc(1, sizeof(ASTNode) + dataCount * sizeof(ASTNodeData));
    CF_ALLOC_CHECK_RN(newNode);

    newNode->parent = activeAst;
    activeAst->data[listDataIndex].astPtr = newNode;
    newNode->actionType = type;
    newNode->dataCount = dataCount;

    activeAst = newNode;
    return newNode;
}

ASTNode *cf_ast_add_leaf(ASTNewNodeTarget target, ASTNodeType type, ASTNodeData data) {
    CF_ACT_AST_CHECK_RN();
    if (target == AST_ROOT) {
        cf_error = CF_ERROR_INVALID_AST_TARGET;
        return NULL;
    }

    ASTNode *newNode = calloc(1, sizeof(ASTNode) + 1 * sizeof(ASTNodeData));
    CF_ALLOC_CHECK_RN(newNode);

    if (target == AST_LEFT_OPERAND || target == AST_UNARY_OPERAND) {
        activeAst->left = newNode;
    } else {
        activeAst->right = newNode;
    }

    newNode->parent = activeAst;
    newNode->actionType = type;
    newNode->dataCount = 1;
    newNode->data[0] = data;

    return newNode;
}

ASTNode *cf_ast_add_leaf_for_list(ASTNodeType type, ASTNodeData data, int listDataIndex) {
    CF_ACT_AST_CHECK_RN();
    if (activeAst->actionType != AST_LIST) {
        cf_error = CF_ERROR_INVALID_AST_TYPE;
        return NULL;
    }

    if (listDataIndex == -1) {
        listDataIndex = activeAst->dataPointerIndex++; // NOLINT(cppcoreguidelines-narrowing-conversions)
    }

    ASTNode *newNode = calloc(1, sizeof(ASTNode) + 1 * sizeof(ASTNodeData));
    CF_ALLOC_CHECK_RN(newNode);

    activeAst->data[listDataIndex].astPtr = newNode;

    newNode->parent = activeAst;
    newNode->actionType = type;
    newNode->dataCount = 1;
    newNode->data[0] = data;

    return newNode;
}

ASTNode *cf_ast_current() {
    return activeAst;
}

void cf_ast_set_current(ASTNode *node) {
    activeAst = node;
}

ASTNode *cf_ast_parent() {
    CF_ACT_AST_CHECK_RN();
    activeAst = activeAst->parent;
    return activeAst;
}

ASTNode *cf_ast_list_root() {
    CF_ACT_AST_CHECK_RN();

    ASTNode *n = ast_get_list_root(activeAst);
    activeAst = n;

    return n;
}

bool cf_ast_is_root() {
    if (activeAst == NULL) {
        cf_error = CF_ERROR_NO_ACTIVE_AST;
        return false;
    }

    return activeAst->parent == NULL;
}

ASTNode *cf_ast_root() {
    CF_ACT_AST_CHECK_RN();

    while (activeAst != NULL) {
        activeAst = activeAst->parent;
    }

    return activeAst;
}

void cf_ast_set_data(unsigned position, ASTNodeData data) {
    CF_ACT_AST_CHECK();
    activeAst->data[position] = data;
}

unsigned cf_ast_push_data(ASTNodeData data) {
    cf_ast_set_data(activeAst->dataPointerIndex, data);
    return activeAst->dataPointerIndex++;
}
