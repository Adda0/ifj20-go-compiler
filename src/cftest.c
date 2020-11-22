#include "control_flow.h"
#include "code_generator.h"
#include <stdlib.h>

int main() {
    cf_init();

    cf_make_function("main");
    cf_add_return_value(NULL, CF_INT);

    cf_make_next_statement(CF_BASIC);

    cf_ast_init(AST_ROOT, AST_DEFINE, 0);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTablePtr = NULL});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});
    cf_use_ast(CF_STATEMENT_BODY);

    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_ASSIGN, 0);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTablePtr = NULL});
    cf_ast_init(AST_RIGHT_OPERAND, AST_MULTIPLY, 0);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTablePtr = NULL});
    cf_ast_init(AST_RIGHT_OPERAND, AST_FUNC_CALL, 0);
    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTablePtr = NULL});

    cf_ast_root();
    cf_use_ast(CF_STATEMENT_BODY);

    cf_make_next_statement(CF_IF);

    cf_ast_init(AST_ROOT, AST_LOG_LT, 0);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 5});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});

    cf_use_ast(CF_IF_CONDITIONAL);

    cf_make_if_then_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_FUNC_CALL, 0);
    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTablePtr = NULL});

    cf_use_ast(CF_STATEMENT_BODY);

    cf_pop_previous_branched_statement();
    cf_make_if_else_statement(CF_RETURN);
    // AST (AST_LIST) has been created automatically
    cf_ast_init_for_list(AST_CONST_INT, 1, 0);
    cf_ast_push_data((ASTNodeData) {.intConstantValue = 1});

    cf_pop_previous_branched_statement();

    cf_make_next_statement(CF_RETURN);
    cf_ast_init_for_list(AST_ID, 1, 0);
    cf_ast_push_data((ASTNodeData) {.symbolTablePtr = NULL});

    tcg_generate();

    return 0;
}