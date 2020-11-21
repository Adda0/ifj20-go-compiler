/** @file control_flow.c
 *
 * IFJ20 compiler
 *
 * @brief Contains definitions for the control flow graph.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#include "control_flow.h"
#include <stdlib.h>
#include <string.h>

#define CF_ALLOC_CHECK(ptr) if ((ptr) == NULL) { currentError = CF_ERROR_INTERNAL; return; }
#define CF_ALLOC_CHECK_RN(ptr) if ((ptr) == NULL) { currentError = CF_ERROR_INTERNAL; return NULL; }
#define CF_ACT_FUN_CHECK() if (activeFunc == NULL) { currentError = CF_ERROR_NO_ACTIVE_FUNCTION; return; }
#define CF_ACT_FUN_CHECK_RN() if (activeFunc == NULL) { currentError = CF_ERROR_NO_ACTIVE_FUNCTION; return NULL; }
#define CF_ACT_STAT_CHECK() if (activeStat == NULL) { currentError = CF_ERROR_NO_ACTIVE_STATEMENT; return; }
#define CF_ACT_STAT_CHECK_RN() if (activeStat == NULL) { currentError = CF_ERROR_NO_ACTIVE_STATEMENT; return NULL; }
#define CF_ACT_AST_CHECK() if (activeAst == NULL) { currentError = CF_ERROR_NO_ACTIVE_AST; return; }
#define CF_ACT_AST_CHECK_RN() if (activeAst == NULL) { currentError = CF_ERROR_NO_ACTIVE_AST; return NULL; }

static struct program_structure *program;
CFError currentError = CF_NO_ERROR;

CFStatement *activeStat;
CFFunction *activeFunc;
ASTNode *activeAst;

struct program_structure *get_program() {
    return program;
}

// Returns the current error state.
CFError cf_error() {
    return currentError;
}

// Initializes the control flow graph generator.
void cf_init() {
    program = malloc(sizeof(struct program_structure));
    program->mainFunc = NULL;
    program->functionList = NULL;
}

// Finds a function, that is already present in the CF graph, and returns a pointer to it.
// If setActive is true, sets it as the active function.
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

// Creates a function and sets it as the active function.
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
            currentError = CF_ERROR_MAIN_REDEFINITION;
            return NULL;
        }
    }

    activeStat = NULL;
    activeFunc = newFunctionNode;

    return newFunctionNode;
}

// Adds an argument to the active function.
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

// Adds a return value to the active function.
// The name parameter may be NULL, in that case, all following parameters must also be unnamed.
// An error is thrown when trying to add a named return value to a function with unnamed return values and vice-versa.
void cf_add_return_value(const char *name, CFDataType type) {
    CF_ACT_FUN_CHECK();

    CFVarListNode *n = activeFunc->returnValues;

    if (n != NULL) {
        if (n->variable.name != NULL && name == NULL || n->variable.name == NULL && name != NULL) {
            currentError = CF_ERROR_RETURN_VALUES_NAMING_MISMATCH;
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

// Creates a statement in the current function and sets it as the active statement.
// Links it with the function and the previous active statement.
// If the statementType is RETURN, creates a new AST_LIST type AST with the amount of data nodes
// equal to the number of arguments of the current function.
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
            newStat->data.bodyAst = cf_ast_init(AST_ROOT, AST_LIST, activeFunc->returnValuesCount);
            break;
        default:
            currentError = CF_ERROR_INVALID_ENUM_VALUE;
            return NULL;
    }

    activeStat = newStat;
    return newStat;
}

/* Uses the active AST as the AST of the active statement.
 * If the active statement is a basic statement:
 *  - The target parameter must be STATEMENT_BODY.
 *  - The type of the AST must be DEFINE, ASSIGN or FUNC_CALL.
 *
 * If the active statement is a FOR statement:
 *  - The target AST is determined using the target parameter, which must NOT be STATEMENT_BODY.
 *  - For FOR_DEFINITION target, the type of the AST must be DEFINE.
 *  - For FOR_CONDITIONAL target, the type must be FUNC_CALL, ID, CONST_BOOL or one of the LOGIC group.
 *  - For FOR_AFTERTHOUGHT target, the type must be ASSIGN.
 *
 * If the active statement is an IF statement:
 *  - The target parameter must be IF_CONDITIONAL.
 *  - The type of the AST must be FUNC_CALL, ID, CONST_BOOL or one of the LOGIC group.
 *
 * If the active statement is a RETURN statement:
 *  - The target parameter must be RETURN_LIST.
 *  - The type of the AST must be AST_LIST.
**/
void cf_use_ast(CFASTTarget target) {
    CF_ACT_STAT_CHECK();
    CF_ACT_AST_CHECK();

    // TODO: this is a bit spaghetti, a cleanup would be nice
    switch (activeStat->statementType) {
        case CF_BASIC:
            if (target != CF_STATEMENT_BODY) {
                currentError = CF_ERROR_INVALID_AST_TARGET;
                return;
            }

            if (activeAst->actionType != AST_DEFINE
                && activeAst->actionType != AST_ASSIGN
                && activeAst->actionType != AST_FUNC_CALL) {
                currentError = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.bodyAst = activeAst;
            break;
        case CF_IF:
            if (target != CF_IF_CONDITIONAL) {
                currentError = CF_ERROR_INVALID_AST_TARGET;
                return;
            }

            if (activeAst->actionType != AST_FUNC_CALL
                && activeAst->actionType != AST_ID
                && activeAst->actionType != AST_CONST_BOOL
                && !(activeAst->actionType >= AST_LOGIC && activeAst->actionType < AST_CONTROL)) {
                currentError = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.ifData->conditionalAst = activeAst;
            break;
        case CF_FOR:
            switch (target) {
                case CF_FOR_DEFINITION:
                    if (activeAst->actionType != AST_DEFINE) {
                        currentError = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->definitionAst = activeAst;
                    break;
                case CF_FOR_CONDITIONAL:
                    if (activeAst->actionType != AST_FUNC_CALL
                        && activeAst->actionType != AST_ID
                        && activeAst->actionType != AST_CONST_BOOL
                        && !(activeAst->actionType >= AST_LOGIC && activeAst->actionType < AST_CONTROL)) {
                        currentError = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->conditionalAst = activeAst;
                    break;
                case CF_FOR_AFTERTHOUGHT:
                    if (activeAst->actionType != AST_ASSIGN) {
                        currentError = CF_ERROR_INVALID_AST_TYPE;
                        return;
                    }
                    activeStat->data.forData->afterthoughtAst = activeAst;
                    break;
                default:
                    currentError = CF_ERROR_INVALID_AST_TARGET;
                    return;
            }
            break;
        case CF_RETURN:
            if (target != CF_RETURN_LIST) {
                currentError = CF_ERROR_INVALID_AST_TARGET;
                return;
            }

            if (activeAst->actionType != AST_LIST) {
                currentError = CF_ERROR_INVALID_AST_TYPE;
                return;
            }

            activeStat->data.bodyAst = activeAst;
            break;
    }
}

// Finds the closest parent IF or FOR statement and sets it as the active statement.
CFStatement *cf_pop_previous_branched_statement() {
    if (activeStat == NULL) return NULL;
    CFStatement *n = activeStat->parentStatement;
    while (n->parentStatement->statementType != CF_IF) {
        n = n->parentStatement;

        if (n == NULL) {
            return NULL;
        }
    }

    activeStat = n;
    return n;
}

// Creates a statement and sets it as the THEN branch statement for the currently active IF statement.
// If the active statement is not an IF statement, throws an error.
CFStatement *cf_make_if_then_statement(CFStatementType type) {
    if (activeStat->statementType != CF_IF) {
        currentError = CF_ERROR_INVALID_OPERATION;
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

// Creates a statement and sets it as the ELSE branch statement for the currently active IF statement.
// If the active statement is not an IF statement, throws an error.
CFStatement *cf_make_if_else_statement(CFStatementType type) {
    if (activeStat->statementType != CF_IF) {
        currentError = CF_ERROR_INVALID_OPERATION;
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

// Creates a statement and sets it as the body statement for the currently active FOR statement.
// If the active statement is not an FOR statement, throws an error.
CFStatement *cf_make_for_body_statement(CFStatementType type) {
    if (activeStat->statementType != CF_FOR) {
        currentError = CF_ERROR_INVALID_OPERATION;
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

// Creates a new AST and sets it as the active AST node.
// If target is not ROOT, links it to the currently active AST node.
ASTNode *cf_ast_init(ASTNewNodeTarget target, ASTNodeType type, unsigned dataCount) {
    if (target != AST_ROOT) {
        CF_ACT_AST_CHECK_RN();
    }

    return NULL; // TODO
}

// Create a new leaf AST with one-length data, links it to the currently active AST node and DOES NOT make it active.
// The target parameter must NOT be ROOT.
ASTNode *cf_ast_add_leaf(ASTNewNodeTarget target, ASTNodeType type, ASTNodeData data) {
    CF_ACT_AST_CHECK_RN();

    return NULL; // TODO
}

// Returns the current active AST node.
ASTNode *cf_ast_current() {
    return activeAst;
}

// Changes the active AST node.
void cf_ast_set_current(ASTNode *node) {
    activeAst = node;
}

// Changes the active AST node to the parent of the currently active AST node.
ASTNode *cf_ast_parent() {
    CF_ACT_AST_CHECK_RN();
    activeAst = activeAst->parent;
}

// Returns true if the active AST node is a root node.
bool cf_ast_is_root() {
    if (activeAst == NULL) {
        currentError = CF_ERROR_NO_ACTIVE_AST;
        return false;
    }

    return activeAst->parent == NULL;
}

// Finds the root of the AST and sets it as the active node.
ASTNode *cf_ast_root() {
    CF_ACT_AST_CHECK_RN();
    return NULL; // TODO
}

// Sets the data of the active AST node on the specified position.
void cf_ast_set_data(unsigned position, ASTNodeData data) {
    CF_ACT_AST_CHECK();
}

// Sets the first position of the active AST node data that hasn't been yet set.
// Returns the set position.
unsigned cf_ast_push_data(ASTNodeData data) {
    cf_ast_set_data(activeAst->dataPointerIndex, data);
    return activeAst->dataPointerIndex;
}