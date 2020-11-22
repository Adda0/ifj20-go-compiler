#include "control_flow.h"
#include "code_generator.h"
#include <stdlib.h>

int main() {
    cf_init();

    cf_make_function("main");
    cf_add_return_value(NULL, CF_INT);

    cf_make_next_statement(CF_BASIC);

    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = NULL});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});
    cf_use_ast(CF_STATEMENT_BODY);

    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_ASSIGN);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = NULL});
    cf_ast_init(AST_RIGHT_OPERAND, AST_MULTIPLY);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = NULL});
    cf_ast_init(AST_RIGHT_OPERAND, AST_FUNC_CALL);
    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = NULL});

    cf_ast_root();
    cf_use_ast(CF_STATEMENT_BODY);

    cf_make_next_statement(CF_IF);

    cf_ast_init(AST_ROOT, AST_LOG_LT);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 5});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});

    cf_use_ast(CF_IF_CONDITIONAL);

    cf_make_if_then_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_FUNC_CALL);
    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = NULL});

    cf_use_ast(CF_STATEMENT_BODY);

    cf_pop_previous_branched_statement();
    cf_make_if_else_statement(CF_RETURN);
    // AST (AST_LIST) has been created automatically
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 1}, 0);

    cf_pop_previous_branched_statement();

    cf_make_next_statement(CF_FOR);
    cf_ast_init(AST_ROOT, AST_ASSIGN);

    cf_make_next_statement(CF_RETURN);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = NULL}, 0);

    tcg_generate();

    return 0;
}