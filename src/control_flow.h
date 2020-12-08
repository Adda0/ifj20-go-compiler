/** @file control_flow.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of functions and data types for the control flow graph.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#ifndef _CONTROL_FLOW_H
#define _CONTROL_FLOW_H

#include <stdbool.h>
#include <stdint.h>
#include "symtable.h"
#include "ast.h"

typedef STDataType CFDataType;

typedef struct cfgraph_variable {
    const char *name;
    CFDataType dataType;
    unsigned position;
} CFVariable;

struct cfgraph_function;
struct cfgraph_statement;

typedef enum cfgraph_statement_type {
    CF_BASIC,
    CF_IF,
    CF_FOR,
    CF_RETURN
} CFStatementType;

typedef struct cfgraph_if_statement {
    ASTNode *conditionalAst;
    struct cfgraph_statement *thenStatement;
    struct cfgraph_statement *elseStatement;
} CFStatementIf;

typedef struct cfgraph_for_statement {
    ASTNode *definitionAst;
    ASTNode *conditionalAst;
    ASTNode *afterthoughtAst;
    struct cfgraph_statement *bodyStatement;
} CFStatementFor;

typedef struct cfgraph_statement {
    struct cfgraph_function *parentFunction;
    struct cfgraph_statement *parentStatement;
    struct cfgraph_statement *parentBranchStatement; // TODO: use this instead of popCount
    struct cfgraph_statement *followingStatement;

    SymbolTable *localSymbolTable;
    int popCount;

    CFStatementType statementType;
    union {
        ASTNode *bodyAst;
        CFStatementIf *ifData;
        CFStatementFor *forData;
    } data;
} CFStatement;

typedef struct cfgraph_variable_list_node {
    struct cfgraph_variable_list_node *previous;
    struct cfgraph_variable_list_node *next;
    CFVariable variable;
} CFVarListNode;

typedef struct cfgraph_function {
    const char *name;
    unsigned argumentsCount;
    unsigned returnValuesCount;

    struct cfgraph_variable_list_node *arguments;
    struct cfgraph_variable_list_node *returnValues;
    CFStatement *rootStatement;

    SymbolTable *symbolTable;
} CFFunction;

typedef struct cfgraph_functions_list_node {
    struct cfgraph_functions_list_node *previous;
    struct cfgraph_functions_list_node *next;
    CFFunction fun;
} CFFuncListNode;

typedef struct cfgraph_program_structure {
    CFFunction *mainFunc;
    SymbolTable *globalSymtable;
    struct cfgraph_functions_list_node *functionList;
} CFProgram;

typedef enum cfgraph_ast_target {
    CF_STATEMENT_BODY,
    CF_FOR_DEFINITION,
    CF_FOR_CONDITIONAL,
    CF_FOR_AFTERTHOUGHT,
    CF_IF_CONDITIONAL,
    CF_RETURN_LIST
} CFASTTarget;

typedef enum ast_new_node_target {
    AST_LEFT_OPERAND,
    AST_RIGHT_OPERAND,
    AST_UNARY_OPERAND,
    AST_ROOT
} ASTNewNodeTarget;

typedef enum cfgraph_error {
    CF_NO_ERROR = 0,
    CF_ERROR_INVALID_AST_TARGET,
    CF_ERROR_INVALID_AST_TYPE,
    CF_ERROR_MAIN_REDEFINITION,
    CF_ERROR_RETURN_VALUES_NAMING_MISMATCH,
    CF_ERROR_INTERNAL,
    CF_ERROR_INVALID_ENUM_VALUE,
    CF_ERROR_INVALID_OPERATION,
    CF_ERROR_NO_ACTIVE_AST,
    CF_ERROR_NO_ACTIVE_STATEMENT,
    CF_ERROR_NO_ACTIVE_FUNCTION,
    CF_ERROR_MAIN_NO_ARGUMENTS_OR_RETURN_VALUES,
    CF_ERROR_SYMTABLE_ALREADY_ASSIGNED,
    CF_ERROR_SYMTABLE_TARGET_HAS_CHILDREN
} CFError;

CFProgram *get_program();

// Holds the current error state. The "no error" state is guaranteed to be a zero,
// so an error check may be performed using `if (cf_error)`.
extern CFError cf_error;

// Initializes the control flow graph generator.
void cf_init();

/* Recursively walks trough the generated program and frees:
 *  - Memory assigned to AST_CONST_STRING data, pointed to by the stringConstantValue pointer.
 *  - Memory occupied by all AST nodes, CFG statement nodes, CFG function nodes and CFG root program node.
 *  - Memory occupied by all symbol tables.
 */
void cf_clean_all();

// Assigns a pointer to the global symbol table.
// This can only be done ONCE!
void cf_assign_global_symtable(SymbolTable *symbolTable);

// Finds a function, that is already present in the CF graph, and returns a pointer to it.
// If setActive is true, sets it as the active function.
CFFunction *cf_get_function(const char *name, bool setActive);

// Creates a function and sets it as the active function.
// Clears the active statement.
CFFunction *cf_make_function(const char *name);

// Assigns a pointer to a symbol table to the active function.
// This can only be on a function with NO root statement!
void cf_assign_function_symtable(SymbolTable *symbolTable);

// Adds an argument to the active function.
void cf_add_argument(const char *name, CFDataType type);

// Adds a return value to the active function.
// The name parameter may be NULL, in that case, all following parameters must also be unnamed.
// An error is thrown when trying to add a named return value to a function with unnamed return values and vice-versa.
void cf_add_return_value(const char *name, CFDataType type);

// Creates a statement in the current function and sets it as the active statement.
// Links it with the function and the previous active statement.
// The created statement inherits its parent's symbol table. If this is a root statement of the active function,
// or when its parent has no symbol table assigned, it inherits the function's top-most global table.
// If the statementType is RETURN, creates a new AST_LIST type AST with the amount of data nodes
// equal to the number of arguments of the current function.
CFStatement *cf_make_next_statement(CFStatementType statementType);

// Assigns a pointer to a symbol table to the active statement.
// This can only be done on a statement with NO following statement!
void cf_assign_statement_symtable(SymbolTable *symbolTable);

// Deprecated.
// Uses the specified AST as the AST of the active statement.
// The semantics of cf_use_ast_explicit() apply.
void cf_use_ast(CFASTTarget target);

/* Uses the specified AST as the AST of the active statement.
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
void cf_use_ast_explicit(ASTNode *node, CFASTTarget target);

// Finds the closest parent IF or FOR statement and sets it as the active statement.
CFStatement *cf_pop_previous_branched_statement();

// Creates a statement and sets it as the THEN branch statement for the currently active IF statement.
// If the active statement is not an IF statement, throws an error.
CFStatement *cf_make_if_then_statement(CFStatementType statementType);

// Creates a statement and sets it as the ELSE branch statement for the currently active IF statement.
// If the active statement is not an IF statement, throws an error.
CFStatement *cf_make_if_else_statement(CFStatementType statementType);

// Creates a statement and sets it as the body statement for the currently active FOR statement.
// If the active statement is not an FOR statement, throws an error.
CFStatement *cf_make_for_body_statement(CFStatementType statementType);

// Checks whether a statement has effect.
bool is_statement_empty(CFStatement *stat);

// ---- Deprecated functions ----

// Creates a new AST and sets it as the active AST node.
// If target is not ROOT, links it to the currently active AST node.
ASTNode *cf_ast_init_with_data(ASTNewNodeTarget target, ASTNodeType type, unsigned dataCount);

inline ASTNode *cf_ast_init(ASTNewNodeTarget target, ASTNodeType type) {
    return cf_ast_init_with_data(target, type, 0);
}

// Creates a new AST meant for being included as data for the currently active AST node of type AST_LIST.
// Sets the newly created AST as the active AST node and assigns it to the previously active AST_LIST node
// at the specified data index. Setting the data index parameter to -1 will instead push the data,
// similarly to cf_ast_push_data().
// If the currently active AST node is not an AST_LIST, throws an error.
ASTNode *cf_ast_init_for_list_with_data(ASTNodeType type, unsigned dataCount, int listDataIndex);

inline ASTNode *cf_ast_init_for_list(ASTNodeType type, int listDataIndex) {
    return cf_ast_init_for_list_with_data(type, 0, listDataIndex);
}

// Create a new leaf AST with one-length data, links it to the currently active AST node and DOES NOT make it active.
// The target parameter must NOT be ROOT.
ASTNode *cf_ast_add_leaf(ASTNewNodeTarget target, ASTNodeType type, ASTNodeData data);

// Create a new leaf AST with one-length data, links it to the currently active AST_LIST node's data
// and DOES NOT make it active. The semantics is similar to cf_ast_init_for_list().
ASTNode *cf_ast_add_leaf_for_list(ASTNodeType type, ASTNodeData data, int listDataIndex);

// Returns the current active AST node.
ASTNode *cf_ast_current();

// Changes the active AST node.
void cf_ast_set_current(ASTNode *node);

// Changes the active AST node to the parent of the currently active AST node.
ASTNode *cf_ast_parent();

ASTNode *cf_ast_list_root();

// Returns true if the active AST node is a root node.
bool cf_ast_is_root();

// Finds the root of the AST and sets it as the active node.
ASTNode *cf_ast_root();

// Sets the data of the active AST node on the specified position.
void cf_ast_set_data(unsigned position, ASTNodeData data);

// Sets the first position of the active AST node data that hasn't been yet set.
// Returns the set position.
unsigned cf_ast_push_data(ASTNodeData data);

#endif //_COMPILER_CONTROL_FLOW_H
