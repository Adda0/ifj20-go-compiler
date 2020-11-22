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

static struct program_structure *program;
CFError cf_error = CF_NO_ERROR;

CFStatement *activeStat;
CFFunction *activeFunc;
ASTNode *activeAst;

struct program_structure *get_program() {
    return program;
}

void cf_init() {
    program = malloc(sizeof(struct program_structure));
    CF_ALLOC_CHECK(program);

    program->mainFunc = NULL;
    program->functionList = NULL;
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

    newFunctionNode->name = name;

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

    CFVarListNode *n = activeFunc->arguments;
    CFVarListNode *newNode = malloc(sizeof(CFVarListNode));
    CF_ALLOC_CHECK(newNode);

    activeFunc->arguments = newNode;
    if (n != NULL) n->previous = newNode;
    newNode->next = n;
    newNode->previous = NULL;

    newNode->variable.name = name;
    newNode->variable.dataType = type;
    newNode->variable.position = activeFunc->argumentsCount;
    activeFunc->argumentsCount++;
}

void cf_add_return_value(const char *name, CFDataType type) {
    CF_ACT_FUN_CHECK();

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

    newNode->variable.name = name;
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

    if (activeFunc->rootStatement == NULL) {
        activeFunc->rootStatement = newStat;
    }

    if (activeStat != NULL) {
        activeStat->followingStatement = newStat;
    }

    switch (statementType) {
        case CF_BASIC:
            break;
        case CF_IF:
            newStat->data.ifData = calloc(1, sizeof(CFStatementIf));
            break;
        case CF_FOR:
            newStat->data.forData = calloc(1, sizeof(CFStatementFor));
            break;
        case CF_RETURN:
            newStat->data.bodyAst = cf_ast_init_with_data(AST_ROOT, AST_LIST, activeFunc->returnValuesCount);
            break;
        default:
            cf_error = CF_ERROR_INVALID_ENUM_VALUE;
            return NULL;
    }

    activeStat = newStat;
    return newStat;
}

void cf_assign_symtable(SymbolTable *symbolTable) {
    CF_ACT_STAT_CHECK();
    activeStat->localSymbolTable = symbolTable;
}

void cf_use_ast(CFASTTarget target) {
    CF_ACT_STAT_CHECK();
    CF_ACT_AST_CHECK();

    // TODO: this is a bit spaghetti, a cleanup would be nice
    switch (activeStat->statementType) {
        case CF_BASIC:
            if (target != CF_STATEMENT_BODY) {
                cf_error = CF_ERROR_INVALID_AST_TARGET;
                return;
            }

            if (activeAst->actionType != AST_DEFINE
                && activeAst->actionType != AST_ASSIGN
                && activeAst->actionType != AST_FUNC_CALL) {
                cf_error = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.bodyAst = activeAst;
            break;
        case CF_IF:
            if (target != CF_IF_CONDITIONAL) {
                cf_error = CF_ERROR_INVALID_AST_TARGET;
                return;
            }

            if (activeAst->actionType != AST_FUNC_CALL
                && activeAst->actionType != AST_ID
                && activeAst->actionType != AST_CONST_BOOL
                && !(activeAst->actionType >= AST_LOGIC && activeAst->actionType < AST_CONTROL)) {
                cf_error = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.ifData->conditionalAst = activeAst;
            break;
        case CF_FOR:
            switch (target) {
                case CF_FOR_DEFINITION:
                    if (activeAst->actionType != AST_DEFINE) {
                        cf_error = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->definitionAst = activeAst;
                    break;
                case CF_FOR_CONDITIONAL:
                    if (activeAst->actionType != AST_FUNC_CALL
                        && activeAst->actionType != AST_ID
                        && activeAst->actionType != AST_CONST_BOOL
                        && !(activeAst->actionType >= AST_LOGIC && activeAst->actionType < AST_CONTROL)) {
                        cf_error = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->conditionalAst = activeAst;
                    break;
                case CF_FOR_AFTERTHOUGHT:
                    if (activeAst->actionType != AST_ASSIGN) {
                        cf_error = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->afterthoughtAst = activeAst;
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

            if (activeAst->actionType != AST_LIST) {
                cf_error = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.bodyAst = activeAst;
            break;
    }
}

CFStatement *cf_pop_previous_branched_statement() {
    if (activeStat == NULL) return NULL;
    CFStatement *n = activeStat->parentStatement;
    while (n->statementType != CF_IF) {
        n = n->parentStatement;

        if (n == NULL) {
            return NULL;
        }
    }

    activeStat = n;
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

    // Restore the previously active statement's follower and set the newly created one as thenStatement instead
    currentActive->followingStatement = currentFollowing;
    currentActive->data.forData->bodyStatement = newStat;

    return newStat;
}

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

    ASTNode *n = activeAst;
    while (n != NULL) {
        if (n->parent->actionType == AST_LIST) {
            for (unsigned i = 0; i < n->parent->dataCount; i++) {
                if (n->parent->data[i].astPtr == n) {
                    activeAst = n->parent;
                    return activeAst;
                }
            }
        }

        n = n->parent;
    }

    return NULL;
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
