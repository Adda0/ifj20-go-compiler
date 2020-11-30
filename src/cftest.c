#include "control_flow.h"
#include "code_generator.h"
#include "symtable.h"
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>

CompilerResult compiler_result = COMPILER_RESULT_SUCCESS;

void test1() {
    SymbolTable *globSt = symtable_init(32);
    SymbolTable *mainSt = symtable_init(32);
    SymbolTable *funGSt = symtable_init(32);
    SymbolTable *funFSt = symtable_init(32);

    // func main()
    STItem *globMainIt = symtable_add(globSt, "main", ST_SYMBOL_FUNC);
    globMainIt->data.reference_counter = 1;

    STItem *funMainvarCIt = symtable_add(mainSt, "c", ST_SYMBOL_VAR);
    STSymbol *funMainvarC = &funMainvarCIt->data;
    funMainvarC->data.var_data.type = CF_INT;
    funMainvarC->reference_counter = 1;

    // func g(a int) int
    STItem *globFunGIt = symtable_add(globSt, "g", ST_SYMBOL_FUNC);
    globFunGIt->data.reference_counter = 1;
    globFunGIt->data.data.func_data = (STFunctionData) {.defined = false};

    STItem *funGvarAIt = symtable_add(funGSt, "a", ST_SYMBOL_VAR);
    STSymbol *funGvarA = &funGvarAIt->data;
    funGvarA->data.var_data.type = CF_INT;
    funGvarA->reference_counter = 1;
    funGvarA->data.var_data.is_argument_variable = true;

    // func f(b int) int
    STItem *globFunFIt = symtable_add(globSt, "f", ST_SYMBOL_FUNC);
    globFunFIt->data.reference_counter = 1;
    globFunFIt->data.data.func_data = (STFunctionData) {.defined = false};

    STItem *funFvarBIt = symtable_add(funFSt, "b", ST_SYMBOL_VAR);
    STSymbol *funFvarB = &funFvarBIt->data;
    funFvarB->data.var_data.type = CF_INT;
    funFvarB->reference_counter = 1;
    funFvarB->data.var_data.is_argument_variable = true;

    cf_init();

    cf_assign_global_symtable(globSt);

    // func main()
    cf_make_function("main");
    cf_assign_function_symtable(mainSt);

    // c := f(1) + g(2)
    cf_make_next_statement(CF_BASIC);
    ASTNode *defStat = cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_STATEMENT_BODY);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = funMainvarC});
    cf_ast_init(AST_RIGHT_OPERAND, AST_ADD);
    // f(1)
    cf_ast_init(AST_LEFT_OPERAND, AST_FUNC_CALL);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globFunFIt->data});
    cf_ast_init_with_data(AST_RIGHT_OPERAND, AST_LIST, 1);
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 1}, 0);
    cf_ast_parent();
    cf_ast_parent();
    // g(2)
    cf_ast_init(AST_RIGHT_OPERAND, AST_FUNC_CALL);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globFunGIt->data});
    cf_ast_init_with_data(AST_RIGHT_OPERAND, AST_LIST, 1);
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 2}, 0);

    ast_infer_node_type(defStat); // explicitly run type inference for the define statement

    // _ = f(1)
    cf_make_next_statement(CF_BASIC);
    ASTNode *asgStat = cf_ast_init(AST_ROOT, AST_ASSIGN);
    cf_use_ast(CF_STATEMENT_BODY);
    ASTNode *n = cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = NULL});
    n->inheritedDataType = CF_BLACK_HOLE;

    // f(1)
    cf_ast_init(AST_RIGHT_OPERAND, AST_FUNC_CALL);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globFunFIt->data});
    cf_ast_init_with_data(AST_RIGHT_OPERAND, AST_LIST, 1);
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 1}, 0);

    ast_infer_node_type(asgStat); // explicitly run type inference for the assign statement

    // func g(a int) int
    cf_make_function("g");
    cf_add_argument("a", CF_INT);
    cf_add_return_value(NULL, CF_INT);
    cf_assign_function_symtable(funGSt);

    cf_make_next_statement(CF_RETURN);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = funGvarA}, 0);

    // func f(b int) int
    cf_make_function("f");
    cf_add_argument("b", CF_INT);
    cf_add_return_value(NULL, CF_INT);
    cf_assign_function_symtable(funFSt);

    cf_make_next_statement(CF_RETURN);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = funFvarB}, 0);

    // Simulate end of analysis, we now have full function data
    STParam *funGparA = calloc(4, sizeof(STParam));
    STParam *funFparB = funGparA + 1;
    STParam *funGret = funFparB + 1;
    STParam *funFret = funGret + 1;

    funGparA->type = CF_INT;
    funGparA->id = "a";
    funFparB->type = CF_INT;
    funFparB->id = "b";
    funGret->type = CF_INT;
    funFret->type = CF_INT;

    globFunGIt->data.data.func_data = (STFunctionData) {.defined = true, .params = funGparA, .ret_types = funGret,
            .params_count = 1, .ret_types_count = 1};
    globFunFIt->data.data.func_data = (STFunctionData) {.defined = true, .params = funFparB, .ret_types = funFret,
            .params_count = 1, .ret_types_count = 1};

    ast_set_strict_inference_state(true);
    ast_infer_node_type(defStat); // run type inference for the define statement again
    ast_infer_node_type(asgStat); // run type inference for the assign statement again
}

void test2() {
    ast_set_strict_inference_state(true);

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
    globFunIt->data.data.func_data = (STFunctionData) {.defined = true, .params = NULL, .ret_types = a,
            .ret_types_count = 2, .params_count = 0};

    STParam *b = calloc(2, sizeof(STParam));
    b->type = CF_INT;
    b->id = NULL; // empty return val name
    (b + 1)->type = CF_INT;
    (b + 1)->id = "x";
    globFun2It->data.data.func_data = (STFunctionData) {.defined = true, .params = (b + 1), .ret_types = b,
            .ret_types_count = 1, .params_count= 1};

    // main() local variables
    STItem *mainPIt = symtable_add(mainSt, "p", ST_SYMBOL_VAR);
    STSymbol *mainP = &mainPIt->data;
    mainP->data.var_data.type = CF_INT;
    mainP->reference_counter = 1;
    STItem *mainQIt = symtable_add(mainSt, "q", ST_SYMBOL_VAR);
    STSymbol *mainQ = &mainQIt->data;
    mainQ->data.var_data.type = CF_INT;
    mainQ->reference_counter = 1;

    // fun() (a int, b int) local variables
    STItem *funIIt = symtable_add(funSt, "i", ST_SYMBOL_VAR);
    STSymbol *funI = &funIIt->data;
    funI->data.var_data.type = CF_INT;
    funI->reference_counter = 1;

    STItem *funAIt = symtable_add(funSt, "a", ST_SYMBOL_VAR);
    STSymbol *funA = &funAIt->data;
    funA->data.var_data.type = CF_INT;
    funA->data.var_data.is_return_val_variable = true;
    funA->reference_counter = 1;

    STItem *funBIt = symtable_add(funSt, "b", ST_SYMBOL_VAR);
    STSymbol *funB = &funBIt->data;
    funB->data.var_data.type = CF_INT;
    funB->data.var_data.is_return_val_variable = true;
    funB->reference_counter = 1;

    // fun->for local variables
    STItem *forAIt = symtable_add(forBodySt, "a", ST_SYMBOL_VAR);
    STSymbol *forA = &forAIt->data;
    forA->data.var_data.type = CF_INT;
    forA->reference_counter = 1;

    STItem *forUIt = symtable_add(forBodySt, "u", ST_SYMBOL_VAR);
    STSymbol *forU = &forUIt->data;
    forU->data.var_data.type = CF_BOOL;
    forU->reference_counter = 1;

    STItem *forVIt = symtable_add(forBodySt, "v", ST_SYMBOL_VAR);
    STSymbol *forV = &forVIt->data;
    forV->data.var_data.type = CF_BOOL;
    forV->reference_counter = 1;

    // fun2(x int) (int) local variables
    STItem *fun2XIt = symtable_add(fun2St, "x", ST_SYMBOL_VAR);
    STSymbol *fun2X = &fun2XIt->data;
    fun2X->data.var_data.type = CF_INT;
    fun2X->reference_counter = 1;
    fun2X->data.var_data.is_argument_variable = true;

    cf_init();

    if (cf_error) {
        fprintf(stderr, "Couldn't initialise CFG.");
        exit(1);
    }

    cf_assign_global_symtable(globSt);
    // func fun2(int x) int { return x * 2 }
    cf_make_function("fun2");
    cf_add_argument("x", CF_INT);
    cf_add_return_value(NULL, CF_INT);
    cf_assign_function_symtable(fun2St);

    cf_make_next_statement(CF_RETURN);
    cf_ast_init_for_list(AST_MULTIPLY, 0);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = fun2X});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 2});

    // func fun() (a int, b int)
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

    // then { return 1, b - 2 }
    cf_make_if_then_statement(CF_RETURN);
    cf_assign_statement_symtable(ifElseSt);
    // AST (AST_LIST) has been created automatically
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 1}, 0);
    cf_ast_init_for_list(AST_SUBTRACT, 1);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = funB});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 2});

    //else { print(5.25, "\n") }
    cf_pop_previous_branched_statement();
    cf_make_if_else_statement(CF_BASIC);
    cf_assign_statement_symtable(ifThenSt);
    cf_ast_init(AST_ROOT, AST_FUNC_CALL);
    cf_use_ast(CF_STATEMENT_BODY);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globPrintIt->data});
    cf_ast_init_with_data(AST_RIGHT_OPERAND, AST_LIST, 2);
    cf_ast_add_leaf_for_list(AST_CONST_FLOAT, (ASTNodeData) {.floatConstantValue = 5.25}, 0);
    cf_ast_add_leaf_for_list(AST_CONST_STRING, (ASTNodeData) {.stringConstantValue = "\n"}, 1);

    cf_pop_previous_branched_statement();

    cf_make_next_statement(CF_FOR);
    cf_assign_statement_symtable(forBodySt);

    // for definition (a := 0)
    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_FOR_DEFINITION);

    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 0});

    // for conditional (a < 25)
    cf_ast_init(AST_ROOT, AST_LOG_LT);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 25});
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
    // print("str", a, "str", false, a == 5, a > 10 && a <= 20, "\n")
    cf_ast_init(AST_ROOT, AST_FUNC_CALL);
    cf_use_ast(CF_STATEMENT_BODY);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globPrintIt->data});
    cf_ast_init_with_data(AST_RIGHT_OPERAND, AST_LIST, 7);
    // arg #0: str
    cf_ast_add_leaf_for_list(AST_CONST_STRING, (ASTNodeData) {.stringConstantValue = "The value of A is: "}, 0);
    // arg #1: a
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA}, 1);
    // arg #2: str
    cf_ast_add_leaf_for_list(AST_CONST_STRING, (ASTNodeData) {.stringConstantValue = ". Nice, isn't it? :)\n"}, 2);
    // arg #3: false
    cf_ast_add_leaf_for_list(AST_CONST_BOOL, (ASTNodeData) {.boolConstantValue = false}, 3);
    // arg #4: a == 5
    cf_ast_init_for_list(AST_LOG_EQ, 4);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 5});
    cf_ast_parent();
    // arg #5: a > 10 && a <= 20
    cf_ast_init_for_list(AST_LOG_AND, 5);
    cf_ast_init(AST_LEFT_OPERAND, AST_LOG_GT);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});
    cf_ast_parent();
    cf_ast_init(AST_RIGHT_OPERAND, AST_LOG_LTE);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forA});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 20});
    cf_ast_parent();
    cf_ast_parent();
    // arg #6: "\n"
    cf_ast_add_leaf_for_list(AST_CONST_STRING, (ASTNodeData) {.stringConstantValue = "\n"}, 6);

    // v := false
    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_STATEMENT_BODY);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forV});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_BOOL, (ASTNodeData) {.boolConstantValue = false});

    // u := v && (v || false) && (i == 10)
    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_STATEMENT_BODY);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forU});

    cf_ast_init(AST_RIGHT_OPERAND, AST_LOG_AND);
    cf_ast_init(AST_LEFT_OPERAND, AST_LOG_AND);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forV});
    cf_ast_init(AST_RIGHT_OPERAND, AST_LOG_OR);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = forV});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_BOOL, (ASTNodeData) {.boolConstantValue = false});
    cf_ast_parent();
    cf_ast_parent();
    cf_ast_init(AST_RIGHT_OPERAND, AST_LOG_EQ);
    cf_ast_add_leaf(AST_LEFT_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = funI});
    cf_ast_add_leaf(AST_RIGHT_OPERAND, AST_CONST_INT, (ASTNodeData) {.intConstantValue = 10});

    cf_pop_previous_branched_statement();

    // return i, 0
    cf_make_next_statement(CF_RETURN);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = funI}, 0);
    cf_ast_add_leaf_for_list(AST_CONST_INT, (ASTNodeData) {.intConstantValue = 0}, 1);

    cf_make_function("main");
    cf_assign_function_symtable(mainSt);
    cf_make_next_statement(CF_BASIC);
    cf_ast_init(AST_ROOT, AST_DEFINE);
    cf_use_ast(CF_STATEMENT_BODY);
    cf_ast_init_with_data(AST_LEFT_OPERAND, AST_LIST, 2);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = mainP}, 0);
    cf_ast_add_leaf_for_list(AST_ID, (ASTNodeData) {.symbolTableItemPtr = mainQ}, 1);
    cf_ast_parent();
    cf_ast_init(AST_RIGHT_OPERAND, AST_FUNC_CALL);
    cf_ast_add_leaf(AST_UNARY_OPERAND, AST_ID, (ASTNodeData) {.symbolTableItemPtr = &globFunIt->data});
}

void factorial_rec() {
    SymbolTable *glob = symtable_init(32),
            *factorial = symtable_init(32),
            *factorial_if_then = symtable_init(32),
            *factorial_if_else = symtable_init(32),
            *main = symtable_init(32),
            *main_if_then = symtable_init(32),
            *main_if_else = symtable_init(32),
            *main_if_then_if_then = symtable_init(32),
            *main_if_then_if_else = symtable_init(32);

    // internal: print
    STItem *si_print = symtable_add(glob, "print", ST_SYMBOL_FUNC);
    STSymbol *sym_print = &si_print->data;
    sym_print->reference_counter = 4;
    sym_print->data.func_data.defined = false;

    // internal: inputi
    STItem *si_inputi = symtable_add(glob, "inputi", ST_SYMBOL_FUNC);
    STSymbol *sym_inputi = &si_inputi->data;
    sym_inputi->reference_counter = 1;
    sym_inputi->data.func_data.defined = true;
    symtable_add_ret_type(si_inputi, NULL, CF_INT);
    symtable_add_ret_type(si_inputi, NULL, CF_INT);

    // func factorial(n int) (int)
    STItem *si_factorial = symtable_add(glob, "factorial", ST_SYMBOL_FUNC);
    STSymbol *sym_factorial = &si_factorial->data;
    sym_factorial->reference_counter = 2;
    sym_factorial->data.func_data.defined = false;


    STSymbol *sym_factorial_n = &symtable_add(factorial, "n", ST_SYMBOL_VAR)->data;
    sym_factorial_n->data.var_data = (STVariableData) {.type = CF_INT, .is_return_val_variable = false, .is_argument_variable = true};
    sym_factorial_n->reference_counter = 3;

    STSymbol *sym_factorial_decN = &symtable_add(factorial, "dec_n", ST_SYMBOL_VAR)->data;
    sym_factorial_decN->data.var_data = (STVariableData) {.type = CF_INT, .is_return_val_variable = false, .is_argument_variable = false};
    sym_factorial_decN->reference_counter = 2;

    // func main()
    STItem *si_main = symtable_add(glob, "main", ST_SYMBOL_FUNC);
    STSymbol *sym_main = &si_main->data;
    sym_main->reference_counter = 1;
    sym_main->data.func_data.defined = true;

    STSymbol *sym_main_a = &symtable_add(main, "a", ST_SYMBOL_VAR)->data;
    sym_main_a->data.var_data = (STVariableData) {.type = CF_INT, .is_return_val_variable = false, .is_argument_variable = false};
    sym_main_a->reference_counter = 4;

    STSymbol *sym_main_err = &symtable_add(main, "err", ST_SYMBOL_VAR)->data;
    sym_main_err->data.var_data = (STVariableData) {.type = CF_INT, .is_return_val_variable = false, .is_argument_variable = false};
    sym_main_err->reference_counter = 2;

    STSymbol *sym_main_if_then_if_else_vysl = &symtable_add(main_if_then_if_else, "vysl", ST_SYMBOL_VAR)->data;
    sym_main_if_then_if_else_vysl->data.var_data = (STVariableData) {.type = CF_INT, .is_return_val_variable = false, .is_argument_variable = false};
    sym_main_if_then_if_else_vysl->reference_counter = 2;

    cf_init();
    cf_assign_global_symtable(glob);

#define link() do { n->left = n1; n->right = n2; if (!ast_infer_node_type(n)) { fprintf(stderr, "Inference error (line %d)\n", __LINE__); } } while(0)
#define link_list(i) do { n->data[(i)].astPtr = n1; if (!ast_infer_node_type(n)) { fprintf(stderr, "Inference error (line %d)\n", __LINE__); } } while(0)

    ASTNode *n1, *n2, *n;

    // func main() {
    cf_make_function("main");
    cf_assign_function_symtable(main);
    cf_make_next_statement(CF_BASIC);

    // print("...")
    n1 = ast_leaf_consts("Zadejte cislo pro vypocet faktorialu: ");
    n = ast_node_list(1);
    link_list(0);
    n1 = n;
    n = ast_node_func_call(sym_print, n1);
    cf_use_ast_explicit(n, CF_STATEMENT_BODY);

    // a, err := inputi()
    cf_make_next_statement(CF_BASIC);
    n1 = ast_node_list(0);
    n = ast_node_func_call(sym_inputi, n1);

    n2 = n;
    n = ast_node_list(2);

    // a, err AST_LIST
    n1 = ast_leaf_id(sym_main_a);
    link_list(0);
    n1 = ast_leaf_id(sym_main_err);
    link_list(1);

    n1 = n;
    n = ast_node(AST_DEFINE);
    link();
    cf_use_ast_explicit(n, CF_STATEMENT_BODY);

    // if err == 0
    cf_make_next_statement(CF_IF);
    n2 = ast_leaf_consti(0);
    n1 = ast_leaf_id(sym_main_err);
    n = ast_node(AST_LOG_EQ);
    link();
    cf_use_ast_explicit(n, CF_IF_CONDITIONAL);
    // {
    //   if a < 0
    cf_make_if_then_statement(CF_IF);
    cf_assign_statement_symtable(main_if_then);
    n2 = ast_leaf_consti(0);
    n1 = ast_leaf_id(sym_main_a);
    n = ast_node(AST_LOG_LT);
    link();
    cf_use_ast_explicit(n, CF_IF_CONDITIONAL);
    //   {
    cf_make_if_then_statement(CF_BASIC);
    cf_assign_statement_symtable(main_if_then_if_then);
    //      print("...", "\n")
    n = ast_node_list(2);
    n1 = ast_leaf_consts("Faktorial nejde spocitat!");
    link_list(0);
    n1 = ast_leaf_consts("\n");
    link_list(1);
    n1 = n;
    n = ast_node_func_call(sym_print, n1);
    cf_use_ast_explicit(n, CF_STATEMENT_BODY);
    //   } else {
    cf_pop_previous_branched_statement();
    cf_make_if_else_statement(CF_BASIC);
    cf_assign_statement_symtable(main_if_then_if_else);
    //      vysl := factorial(a)

    // -> funcall argument
    n = ast_node_list(1);
    n1 = ast_leaf_id(sym_main_a);
    link_list(0);

    n2 = ast_node_func_call(sym_factorial, n);
    n1 = ast_leaf_id(sym_main_if_then_if_else_vysl);
    n = ast_node(AST_DEFINE);
    link();
    cf_use_ast_explicit(n, CF_STATEMENT_BODY);

    //     print("...", vysl, "\n")
    cf_make_next_statement(CF_BASIC);
    n = ast_node_list(3);
    n1 = ast_leaf_consts("Vysledek je ");
    link_list(0);
    n1 = ast_leaf_id(sym_main_if_then_if_else_vysl);
    link_list(1);
    n1 = ast_leaf_consts("\n");
    link_list(2);
    n1 = n;
    n = ast_node_func_call(sym_print, n1);
    cf_use_ast_explicit(n, CF_STATEMENT_BODY);

    // } else {
    cf_pop_previous_branched_statement();
    cf_pop_previous_branched_statement();
    cf_make_if_else_statement(CF_BASIC);
    cf_assign_statement_symtable(main_if_else);
    n1 = ast_leaf_consts("Chyba pri nacitani celeho cisla!\n");
    n = ast_node_list(1);
    link_list(0);
    n1 = n;
    n = ast_node_func_call(sym_print, n1);
    cf_use_ast_explicit(n, CF_STATEMENT_BODY);

    // func factorial(n int) (int) {
    cf_make_function("factorial");
    cf_assign_function_symtable(factorial);
    cf_add_argument("n", CF_INT);
    cf_add_return_value(NULL, CF_INT);

    // dec_n := n - 1
    cf_make_next_statement(CF_BASIC);
    n1 = ast_leaf_id(sym_factorial_n);
    n2 = ast_leaf_consti(1);
    n = ast_node(AST_SUBTRACT);
    link();

    n1 = ast_leaf_id(sym_factorial_decN);
    n2 = n;
    n = ast_node(AST_DEFINE);
    link();
    cf_use_ast_explicit(n, CF_STATEMENT_BODY);

    // if n < 2 {
    cf_make_next_statement(CF_IF);
    n1 = ast_leaf_id(sym_factorial_n);
    n2 = ast_leaf_consti(2);
    n = ast_node(AST_LOG_LT);
    link();
    cf_use_ast_explicit(n, CF_IF_CONDITIONAL);
    // return 1
    cf_make_if_then_statement(CF_RETURN);
    cf_assign_statement_symtable(factorial_if_then);
    n1 = ast_leaf_consti(1);
    n = cf_ast_current();
    link_list(0); // link 0 to predefined RETURN's AST_LIST
    // } else {
    cf_pop_previous_branched_statement();
    cf_make_if_else_statement(CF_RETURN);
    cf_assign_statement_symtable(factorial_if_else);
    // return n * factorial(dec_n)

    // -> funcall argument
    n = ast_node_list(1);
    n1 = ast_leaf_id(sym_factorial_decN);
    link_list(0);

    n2 = ast_node_func_call(sym_factorial, n);
    n1 = ast_leaf_id(sym_factorial_n);
    n = ast_node(AST_MULTIPLY);
    link();
    n1 = n;
    n = cf_ast_current();
    link_list(0); // link MULTIPLY to predefined RETURN's AST_LIST

    si_factorial->data.data.func_data.defined = true;
    symtable_add_param(si_factorial, "n", CF_INT);
    symtable_add_ret_type(si_factorial, NULL, CF_INT);

    ast_set_strict_inference_state(true);
}

int main() {
    factorial_rec();
    tcg_generate();

    return compiler_result;
}
