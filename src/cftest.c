#include "control_flow.h"
#include "code_generator.h"
#include "symtable.h"
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>

CompilerResult compiler_result = COMPILER_RESULT_SUCCESS;

int main() {
    SymbolTable *globSt = symtable_init(32);
    SymbolTable *mainSt = symtable_init(32);
    SymbolTable *funSt = symtable_init(32);
    SymbolTable *fun2St = symtable_init(32);
    SymbolTable *ifThenSt = symtable_init(32);
    SymbolTable *ifElseSt = symtable_init(32);
    SymbolTable *forBodySt = symtable_init(32);

    STItem *globMainIt = symtable_add(globSt, "main", ST_SYMBOL_FUNC);
    globMainIt->data.reference_counter = 2;

    STItem *globFunIt = symtable_add(globSt, "fun", ST_SYMBOL_FUNC);
    globFunIt->data.reference_counter = 2;

    STItem *globFun2It = symtable_add(globSt, "fun2", ST_SYMBOL_FUNC);
    globFun2It->data.reference_counter = 2;

    STItem *globPrintIt = symtable_add(globSt, "print", ST_SYMBOL_FUNC);
    globPrintIt->data.reference_counter = 2;

    STParam *a = calloc(2, sizeof(STParam));
    a->type = CF_INT;
    a->id = "a";
    a->next = (a + 1);
    a->next->type = CF_INT;
    a->next->id = "b";
    globFunIt->data.data.func_data = (STFunctionData) {.defined = true, .params = NULL, .ret_types = a};

    STParam *b = calloc(2, sizeof(STParam));
    b->type = CF_INT;
    b->id = NULL; // empty return val name
    (b + 1)->type = CF_INT;
    (b + 1)->id = "x";
    globFun2It->data.data.func_data = (STFunctionData) {.defined = true, .params = (b + 1), .ret_types = b};

    STItem *funIIt = symtable_add(funSt, "i", ST_SYMBOL_VAR);
    STSymbol *funI = &funIIt->data;
    funI->data.var_data.type = CF_INT;

    STItem *funAIt = symtable_add(funSt, "a", ST_SYMBOL_VAR);
    STSymbol *funA = &funAIt->data;
    funA->data.var_data.type = CF_INT;

    STItem *funBIt = symtable_add(funSt, "b", ST_SYMBOL_VAR);
    STSymbol *funB = &funBIt->data;
    funB->data.var_data.type = CF_INT;

    STItem *forAIt = symtable_add(forBodySt, "a", ST_SYMBOL_VAR);
    STSymbol *forA = &forAIt->data;
    forA->data.var_data.type = CF_INT;

    STItem *fun2XIt = symtable_add(fun2St, "x", ST_SYMBOL_VAR);
    STSymbol *fun2X = &fun2XIt->data;
    fun2X->data.var_data.type = CF_INT;

    cf_init();

    if (cf_error) {
        fprintf(stderr, "Couldn't initialise CFG.");
        return 1;
    }

    cf_assign_global_symtable(globSt);
    // func fun2(int x) int { return x * 2 }
    cf_make_function("fun2");
    cf_add_argument("x", CF_INT);
    cf_assign_function_symtable(fun2St);

    cf_make_next_statement(CF_RETURN);
    cf_ast_init_for_list(AST_MULTIPLY, 0);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = fun2X});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 2});

    // func fun() (int, int)
    cf_make_function("fun");
    cf_assign_function_symtable(funSt);

    // i := 10
    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = funI});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});

    // i = a * fun2(a)
    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_ASSIGN);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = funI});
    cf_ast_init(AST_RIGHT_OPERAND, AST_MULTIPLY);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = funA});
    cf_ast_init(AST_RIGHT_OPERAND, AST_FUNC_CALL);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globFun2It->data});
    cf_ast_init_with_data(AST_RIGHT_OPERAND, AST_LIST, 1);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = funA}, 0);

    // if 5 > 10
    cf_make_next_statement(CF_IF);
    // if conditional
    cf_ast_init(AST_ROOT, AST_LOG_GT);
    cf_use_ast(CF_IF_CONDITIONAL);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 5});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});

    // then { fun1() }
    cf_make_if_then_statement(CF_BASIC);
    cf_assign_statement_symtable(ifThenSt);
    cf_ast_init(AST_ROOT, AST_FUNC_CALL);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globFunIt->data});

    //else { return 1, b - 2 }
    cf_pop_previous_branched_statement();
    cf_make_if_else_statement(CF_RETURN);
    cf_assign_statement_symtable(ifElseSt);
    // AST (AST_LIST) has been created automatically
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 1}, 0);
    cf_ast_init_for_list(AST_SUBTRACT, 1);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = funB});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 2});

    cf_pop_previous_branched_statement();

    cf_make_next_statement(CF_FOR);
    cf_assign_statement_symtable(forBodySt);

    // for definition (a := 0)
    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_FOR_DEFINITION);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 0});

    // for conditional (a < 10)
    cf_ast_init(AST_ROOT, AST_LOG_LT);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});
    cf_use_ast(CF_FOR_CONDITIONAL);

    // for afterthought (a = a + 1)
    cf_ast_init(AST_ROOT, AST_ASSIGN);
    cf_use_ast(CF_FOR_AFTERTHOUGHT);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_init(AST_RIGHT_OPERAND, AST_ADD);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 1});

    // for body
    cf_make_for_body_statement(CF_BASIC);
    // print(a)
    cf_ast_init(AST_ROOT, AST_FUNC_CALL);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globPrintIt->data});
    cf_ast_init_with_data(AST_RIGHT_OPERAND, AST_LIST, 1);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA}, 0);

    cf_pop_previous_branched_statement();

    // return i, 0
    cf_make_next_statement(CF_RETURN);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = funI}, 0);
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 0}, 1);

    cf_make_function("main");
    cf_assign_function_symtable(mainSt);
    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_FUNC_CALL);
    cf_use_ast(CF_STATEMENT_BODY);
    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globFunIt->data});

    tcg_generate();

    return compiler_result;
}
