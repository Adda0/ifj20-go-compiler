#include "control_flow.h"
#include "code_generator.h"
#include "symtable.h"
#include <stdio.h>

int main() {
    STItem i = {.key = "i",
            .data = (STSymbol) {.identifier = "i",
                    .type = ST_SYMBOL_VAR,
                    .data = (STSymbolData) {.var_data = (STVariableData) {.type = CF_INT}}}};
    STItem a = {.key = "a",
            .data = (STSymbol) {.identifier = "a",
                    .type = ST_SYMBOL_VAR,
                    .data = (STSymbolData) {.var_data = (STVariableData) {.type = CF_INT}}}};
    STItem b = {.key = "b",
            .data = (STSymbol) {.identifier = "b",
                    .type = ST_SYMBOL_VAR,
                    .data = (STSymbolData) {.var_data = (STVariableData) {.type = CF_INT}}}};
    STItem fun1 = {.key = "fun1",
            .data = (STSymbol) {.identifier = "fun1",
                    .type = ST_SYMBOL_FUNC,
                    .data = (STSymbolData) {.func_data = (STFunctionData) {.defined=true}}}};
    STItem print = {.key = "print",
            .data = (STSymbol) {.identifier = "print",
                    .type = ST_SYMBOL_FUNC,
                    .data = (STSymbolData) {.func_data = (STFunctionData) {.defined=true}}}};
    STItem aFor = {.key = "a",
            .data = (STSymbol) {.identifier = "a",
                    .type = ST_SYMBOL_VAR,
                    .data = (STSymbolData) {.var_data = (STVariableData) {.type = CF_INT}}}};

    cf_init();

    if (cf_error) {
        fprintf(stderr, "Couldn't initialise CFG.");
        return 1;
    }

    // func main()
    cf_make_function("main");
    cf_add_return_value(NULL, CF_INT);
    cf_add_return_value(NULL, CF_INT);

    // i := 10
    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &i});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});

    // i = a * b
    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_ASSIGN);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &i});
    cf_ast_init(AST_RIGHT_OPERAND, AST_MULTIPLY);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &a});
    cf_ast_init(AST_RIGHT_OPERAND, AST_FUNC_CALL);
    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &fun1});

    // if 5 < 10
    cf_make_next_statement(CF_IF);
    // if conditional
    cf_ast_init(AST_ROOT, AST_LOG_LT);
    cf_use_ast(CF_IF_CONDITIONAL);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 5});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});

    // then { fun1() }
    cf_make_if_then_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_FUNC_CALL);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &fun1});

    //else { return 1, b - 2 }
    cf_pop_previous_branched_statement();
    cf_make_if_else_statement(CF_RETURN);
    // AST (AST_LIST) has been created automatically
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 1}, 0);
    cf_ast_init_for_list(AST_SUBTRACT, 1);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) { .symbolTableItemPtr = &b});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 2});

    cf_pop_previous_branched_statement();

    cf_make_next_statement(CF_FOR);

    // for definition (a := 0)
    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_FOR_DEFINITION);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &aFor});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 0});

    // for conditional (a < 10)
    cf_ast_init(AST_ROOT, AST_LOG_LT);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &aFor});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});
    cf_use_ast(CF_FOR_CONDITIONAL);

    // for afterthought (a = a + 1)
    cf_ast_init(AST_ROOT, AST_ASSIGN);
    cf_use_ast(CF_FOR_AFTERTHOUGHT);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &aFor});
    cf_ast_init(AST_RIGHT_OPERAND, AST_ADD);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &aFor});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 1});

    // for body
    cf_make_for_body_statement(CF_BASIC);
    // print(a)
    cf_ast_init(AST_ROOT, AST_FUNC_CALL);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &print});
    cf_ast_init_with_data(AST_RIGHT_OPERAND, AST_LIST, 1);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = &aFor}, 0);

    cf_pop_previous_branched_statement();

    // return i
    cf_make_next_statement(CF_RETURN);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = &i}, 0);
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) { .intConstantValue = 128}, 1);

    tcg_generate();

    return 0;
}
