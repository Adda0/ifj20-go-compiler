/** @file control_flow.h
 *
 * IFJ20 compiler
 *
 * @brief Contains data types and declarations for the control flow graph.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#ifndef _CONTROL_FLOW_H
#define _CONTROL_FLOW_H

#include <stdbool.h>
#include <stdint.h>

typedef enum cfgraph_data_type {
    CF_INT,
    CF_FLOAT,
    CF_STRING,
    CF_BOOL,
    CF_NIL
} CFDataType;

typedef enum ast_node_action_type {
    // ARITHMETIC group
    CF_ADD,
    CF_SUBTRACT,
    CF_MULTIPLY,
    CF_DIVIDE,
    CF_AR_NEGATE,
    // LOGIC group
    CF_LOG_NOT,
    CF_LOG_AND,
    CF_LOG_OR,
    CF_LOG_EQ,
    CF_LOG_LT,
    CF_LOG_GT,
    CF_LOG_LTE,
    CF_LOG_GTE,
    // CONTROL group
    CF_ASSIGN,
    CF_DEFINE,
    CF_FUNC_CALL,
    // VALUE group
    CF_AST_LIST,
    CF_ID,
    CF_CONST_INT,
    CF_CONST_FLOAT,
    CF_CONST_STRING,
    CF_CONST_BOOL
} ASTNodeType;

struct ast_node;

typedef union ast_node_data {
    const void *symbolTablePtr;
    struct ast_node *astPtr;
    int64_t intConstantValue;
    double floatConstantValue;
    const char *stringConstantValue;
    bool boolConstantValue;
} ASTNodeData;

typedef struct ast_node {
    struct ast_node *parent;
    ASTNodeType actionType;
    struct ast_node *left;
    struct ast_node *right;

    unsigned dataCount;
    unsigned dataPointerIndex;
    ASTNodeData data[];
} ASTNode;

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
    struct cfgraph_statement *followingStatement;

    CFStatementType statementType;
    union {
        ASTNode *bodyAst;
        CFStatementIf *ifData;
        CFStatementFor *forData;
    } data;
} CFStatement;

struct cfgraph_variable_list_node {
    struct cfgraph_functions_list_node *previous;
    struct cfgraph_functions_list_node *next;
    CFVariable *variable;
};

typedef struct cfgraph_function {
    const char *name;
    unsigned argumentsCount;
    unsigned returnValuesCount;

    struct cfgraph_variable_list_node *arguments;
    struct cfgraph_variable_list_node *returnValues;
    CFStatement *rootStatement;
} CFFunction;

struct cfgraph_functions_list_node {
    struct cfgraph_functions_list_node *previous;
    struct cfgraph_functions_list_node *next;
    CFFunction *fun;
};

struct program_structure {
    CFFunction *mainFunc;
    struct cfgraph_functions_list_node *functionList;
};

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
    CF_NO_ERROR,
    CF_ERROR_INVALID_AST_TARGET
} CFError;

// Returns the current error state.
CFError cf_error();

// Initializes the control flow graph generator.
void cf_init();

// Finds a function, that is already present in the CF graph, and returns a pointer to it.
// If setActive is true, sets it as the active function.
CFFunction *cf_get_function(const char *name, bool setActive);

// Creates a function and sets it as the active function.
CFFunction *cf_make_function(const char *name);

// Adds an argument to the active function.
void cf_add_argument(const char *name, CFDataType type);

// Adds a return value to the active function.
// The name parameter may be NULL, in that case, all following parameters must also be unnamed.
// An error is thrown when trying to add a named return value to a function with unnamed return values and vice-versa.
void cf_add_return_value(const char *name, CFDataType type);

// Creates a statement in the current function and sets it as the active statement.
// Links it with the function and the previous active statement.
// If the statementType is RETURN, creates a new AST_LIST type AST with the amount of data nodes equal to the number of
// Throws error if the current statement is an IF or a FOR statement.
CFStatement *cf_make_next_statement(CFStatementType statementType);

/* Uses the active AST as the AST of the active statement.
 * If the active statement is a basic statement:
 *  - The target parameter must be STATEMENT_BODY.
 *  - The type of the AST must be DEFINE, ASSIGN or FUNC_CALL.
 *
 * If the active statement is a FOR statement:
 *  - The target AST is determined using the target parameter, which must NOT be STATEMENT_BODY.
 *  - For FOR_DEFINITION target, the type of the AST must be DEFINE.
 *  - For FOR_CONDITIONAL target, the type must be FUNC_CALL or one of the LOGIC group.
 *  - For FOR_AFTERTHOUGHT target, the type must be ASSIGN.
 *
 * If the active statement is an IF statement:
 *  - The target parameter must be IF_CONDITIONAL.
 *  - The type of the AST must be FUNC_CALL or one of the LOGIC group.
 *
 * If the active statement is a RETURN statement:
 *  - The target parameter must be RETURN_LIST.
 *  - The type of the AST must be AST_LIST.
**/
void cf_use_ast(CFASTTarget target);

// Finds the closest parent IF or FOR statement and sets it as the active statement.
CFStatement *cf_pop_previous_branched_statement();

// Creates a statement and sets it as the THEN branch statement for the currently active IF statement.
// If the active statement is not an IF statement, throws an error.
CFStatement *cf_make_if_then_statement();

// Creates a statement and sets it as the ELSE branch statement for the currently active IF statement.
// If the active statement is not an IF statement, throws an error.
CFStatement *cf_make_if_else_statement();

// Creates a statement and sets it as the body statement for the currently active FOR statement.
// If the active statement is not an FOR statement, throws an error.
CFStatement *cf_make_for_body_statement();

// Creates a new AST and sets it as the active AST node.
// If target is not ROOT, links it to the currently active AST node.
ASTNode *cf_ast_init(ASTNewNodeTarget target, ASTNodeType type, unsigned dataCount);

// Create a new leaf AST with one-length data, links it to the currently active AST node and DOES NOT make it active.
// The target parameter must NOT be ROOT.
ASTNode *cf_ast_add_leaf(ASTNewNodeTarget target, ASTNodeType type, ASTNodeData data);

// Returns the current active AST node.
ASTNode *cf_ast_current();

// Changes the active AST node.
void cf_ast_set_current(ASTNode *node);

// Changes the active AST node to the parent of the currently active AST node.
ASTNode *cf_ast_parent();

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
