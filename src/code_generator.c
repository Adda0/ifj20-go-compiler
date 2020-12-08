/** @file code_generator.c
 *
 *
 * IFJ20 compiler
 *
 * @brief Implements the target code generator.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#include <stdio.h>
#include <stdlib.h>
#include "code_generator.h"
#include "stderr_message.h"
#include "mutable_string.h"
#include "symtable.h"
#include "stacks.h"

#define TCG_DEBUG 1
#define UINT_DIGITS 21

#define out_s(s) puts((s))
#define out(...) printf(__VA_ARGS__); putchar('\n')
#define out_nnl(...) printf(__VA_ARGS__)
#define out_nl() putchar('\n')

#define is_direct_ast(ast) ((ast)->actionType > AST_VALUE)

#if TCG_DEBUG
#define dbg(msg, ...) printf("# --> "); printf((msg),##__VA_ARGS__); putchar('\n'); fflush(stdout)
#else
#define dbg(msg, ...)
#endif

#define COND_RES_VAR "GF@$cond_res"
#define COND_LHS_VAR "GF@$cond_lhs"
#define COND_RHS_VAR "GF@$cond_rhs"

#define REG_1 "GF@$r1"
#define REG_2 "GF@$r2"
#define REG_3 "GF@$r3"

// Global variables keeping the current state of the generator
struct {
    CFFunction *function;
    unsigned scopeCounter;
    unsigned jumpingExprCounter;
    unsigned ifCounter;

    bool isMain;
    bool generateMainAsFunction;
    bool isInBranch;
    bool terminated;
    bool terminatedInBranch;

    SymtableStack stStack;
} currentFunction;

struct {
    STSymbol *print;
    STSymbol *int2float;
    STSymbol *float2int;
    STSymbol *len;
    STSymbol *substr;
    STSymbol *ord;
    STSymbol *chr;
    STSymbol *inputs;
    STSymbol *inputi;
    STSymbol *inputf;
    STSymbol *inputb;

    bool reg3Used;
} symbs;

bool onlyFindDefinedSymbols = false;

void generate_statement(CFStatement *stat);

void generate_assignment_for_varname(const char *varName, ASTNode *value);

bool generate_expression_ast_result(ASTNode *exprAst);

bool generate_logic_expression_tree(ASTNode *exprAst, char *trueLabel, char *falseLabel);

bool generate_logic_expression_assignment(ASTNode *exprAst, const char *targetVarName);

#define find_internal_symbol(symbol) (it = symtable_find(globSt, (symbol))) == NULL ? NULL : (&it->data)

// Finds symbols corresponding to the built-in functions in the global symbol table and saves pointers to them.
void find_internal_symbols(SymbolTable *globSt) {
    STItem *it;

    symbs.print = find_internal_symbol("print");
    symbs.int2float = find_internal_symbol("int2float");
    symbs.float2int = find_internal_symbol("float2int");
    symbs.len = find_internal_symbol("len");
    symbs.substr = find_internal_symbol("substr");
    symbs.ord = find_internal_symbol("ord");
    symbs.chr = find_internal_symbol("chr");
    symbs.inputs = find_internal_symbol("inputs");
    symbs.inputi = find_internal_symbol("inputi");
    symbs.inputf = find_internal_symbol("inputf");
    symbs.inputb = find_internal_symbol("inputb");

    symbs.reg3Used =
            (symbs.ord != NULL && symbs.ord->reference_counter > 0)
            || (symbs.substr != NULL && symbs.substr->reference_counter > 0);
}

char *convert_to_target_string_form_cb(const char *input, bool prependType) {
    // Worst case scenario, all characters will be converted to escape sequences which are 4 chars long
    char *buf;
    char *bufRet;
    if (prependType) {
        buf = malloc(7 + strlen(input) * 4 + 1);
        bufRet = buf;
        strcpy(buf, "string@");
        buf += 7;
    } else {
        buf = malloc(strlen(input) * 4 + 1);
        bufRet = buf;
    }

    char n;
    while ((n = *(input++)) != '\0') {
        if (n <= 32 || n == 35 || n == 92) {
            sprintf(buf, "\\%.3i", n);
            buf += 4;
        } else {
            *buf++ = n;
        }
    }

    *buf = '\0';
    return bufRet;
}

char *convert_to_target_string_form(const char *input) {
    return convert_to_target_string_form_cb(input, false);
}

// Finds the first symbol table on the current symbol table stack that contains the specified identifier.
// The global onlyFindDefinedSymbols variable controls whether only variables that have been defined already
// should be returned (this is typically used when evaluating right-hand sides of expressions).
SymbolTable *find_sym_table(const char *id) {
    SymbolTable *tab = NULL;
    symtable_stack_find_symbol_and_symtable(&currentFunction.stStack, id, &tab, onlyFindDefinedSymbols);
    return tab;
}

// Finds an identifier in the symbol table stack and creates a MutableString with the corresponding variable's name
// decorated with the scope (symbol table) number of the table it was found in.
// If the isTF argument is true, it doesn't perform a symbol table lookup and instead decorates the variable name
// with 'TF@$1_' (the top-level symtable of a function will always have number one, assigned in generate_definitions()).
MutableString make_var_name(const char *id, bool isTF) {
    char *i = malloc(UINT_DIGITS);

    if (isTF) {
        sprintf(i, "1");
    } else {
        SymbolTable *symtab = find_sym_table(id);

        if (symtab == NULL) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                           "Symbol '%s' not found.\n", id);
            sprintf(i, "error");
        }

        sprintf(i, "%u", symtab->symbol_prefix);
    }

    MutableString str;
    mstr_make(&str, 5, isTF ? "TF" : "LF", "@$", i, "_", id);
    free(i);
    return str;
}

// Finds an identifier in the symbol table stack, decorates the identifier with LF@$ and the scope (symbol table) number
// of the table it was found in and prints the resulting variable name to output.
void print_var_name_id(const char *id) {
    SymbolTable *symtab = find_sym_table(id);

    if (symtab == NULL) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Symbol '%s' not found.\n", id);
        return;
    }

    out_nnl("%s@$%u_%s", "LF", symtab->symbol_prefix, id);
}

// Calls print_var_name_id with the identifier referenced by the specified AST_ID node.
void print_var_name(ASTNode *idAstNode) {
    STSymbol *st = idAstNode->data[0].symbolTableItemPtr;
    print_var_name_id(st->identifier);
}

// Either prints the target code representation of a constant or calls print_var name to print a decorated variable name.
void print_var_name_or_const(ASTNode *node) {
    if (node->actionType == AST_CONST_INT) {
        out_nnl("int@%li", node->data[0].intConstantValue);
    } else if (node->actionType == AST_CONST_FLOAT) {
        out_nnl("float@%a", node->data[0].floatConstantValue);
    } else if (node->actionType == AST_CONST_BOOL) {
        out_nnl("bool@%s", node->data[0].boolConstantValue ? "true" : "false");
    } else if (node->actionType == AST_CONST_STRING) {
        char *s = convert_to_target_string_form(node->data[0].stringConstantValue);
        out_nnl("string@%s", s);
        free(s);
    } else {
        print_var_name(node);
    }
}

// ---- Built-in functions generators ----
void generate_print_log_expr(ASTNode *ast) {
    unsigned counter = currentFunction.ifCounter;
    currentFunction.ifCounter++;

    char i[UINT_DIGITS];
    sprintf(i, "%u", counter);

    MutableString trueLabelStr, falseLabelStr;

    mstr_make(&trueLabelStr, 5, "$", currentFunction.function->name, "_print", i, "_true");
    mstr_make(&falseLabelStr, 5, "$", currentFunction.function->name, "_print", i, "_false");

    generate_logic_expression_tree(ast, mstr_content(&trueLabelStr),
                                   mstr_content(&falseLabelStr));

    out("LABEL %s", mstr_content(&trueLabelStr));
    out("WRITE bool@true");
    out("JUMP $%s_print%i_end", currentFunction.function->name, counter);
    out("LABEL %s", mstr_content(&falseLabelStr));
    out("WRITE bool@false");
    out("LABEL $%s_print%i_end", currentFunction.function->name, counter);

    mstr_free(&trueLabelStr);
    mstr_free(&falseLabelStr);
}

void generate_print(ASTNode *argAstList) {
    if (is_ast_empty(argAstList))
        return;

    for (unsigned i = 0; i < argAstList->dataCount; i++) {
        ASTNode *ast = argAstList->data[i].astPtr;
        switch (ast->actionType) {
            case AST_CONST_STRING: {
                char *str = convert_to_target_string_form(ast->data[0].stringConstantValue);
                out("WRITE string@%s", str);
                free(str);
            }
                break;
            case AST_CONST_INT:
            out("WRITE int@%li", ast->data[0].intConstantValue);
                break;
            case AST_CONST_FLOAT:
            out("WRITE float@%a", ast->data[0].floatConstantValue);
                break;
            case AST_CONST_BOOL:
            out("WRITE bool@%s", ast->data[0].boolConstantValue ? "true" : "false");
                break;
            case AST_ID:
                out_nnl("WRITE ");
                print_var_name(ast);
                out_nl();
                break;
            default:
                ast_infer_node_type(ast);

                if (ast->inheritedDataType == CF_BOOL) {
                    generate_print_log_expr(ast);

                } else {
                    generate_expression_ast_result(ast);
                    out("POPS %s", REG_1);
                    out("WRITE %s", REG_1);
                }
                break;
        }
    }
}

void generate_internal_inputx(const char *expType, const char *defaultValue) {
    unsigned counter = currentFunction.jumpingExprCounter++;

    // Read value into REG_1
    out("READ %s %s", REG_1, expType);
    // Write its type into REG_2
    out("TYPE %s %s", REG_2, REG_1);

    out("JUMPIFNEQ $%s_input%i_error %s string@%s", currentFunction.function->name, counter,
        REG_2, expType);

    // Type matches -> push result and 0
    out("PUSHS int@0");
    out("PUSHS %s", REG_1);
    out("JUMP $%s_input%i_end", currentFunction.function->name, counter);

    // Type doesn't match -> push default value of target type and 1
    out("LABEL $%s_input%i_error", currentFunction.function->name, counter);
    out("PUSHS int@1");
    out("PUSHS %s@%s", expType, defaultValue);

    out("LABEL $%s_input%i_end", currentFunction.function->name, counter);
}

void generate_internal_int2float(ASTNode *argAst) {
    if (argAst->actionType != AST_LIST || argAst->dataCount != 1) {
        return;
    }

    generate_expression_ast_result(argAst->data[0].astPtr);
    out("INT2FLOATS");
}

void generate_internal_float2int(ASTNode *argAst) {
    if (argAst->actionType != AST_LIST || argAst->dataCount != 1) {
        return;
    }

    generate_expression_ast_result(argAst->data[0].astPtr);
    out("FLOAT2INTS");
}

void generate_internal_len(ASTNode *argAst) {
    if (argAst->actionType != AST_LIST || argAst->dataCount != 1) {
        return;
    }
    argAst = argAst->data[0].astPtr;

    if (argAst->actionType == AST_ID || argAst->actionType == AST_CONST_STRING) {
        out_nnl("STRLEN %s ", REG_1);
        print_var_name_or_const(argAst);
        out_nl();
        out("PUSHS %s", REG_1);
    } else {
        generate_expression_ast_result(argAst);
        out("POPS %s", REG_1);
        out_nnl("STRLEN %s %s", REG_1, REG_1);
        out("PUSHS %s", REG_1);
    }
}

void generate_internal_chr(ASTNode *argAst) {
    if (argAst->actionType != AST_LIST || argAst->dataCount != 1) {
        return;
    }

    argAst = argAst->data[0].astPtr;

    if (argAst->actionType == AST_CONST_INT) {
        int i = argAst->data[0].intConstantValue;
        if (i < 0 || i > 255) {
            out("PUSHS int@1");
            out("PUSHS string@");
            return;
        } else {
            out("PUSHS int@0");
            out("PUSHS string@\\%.3i", i);
            return;
        }
    } else {
        unsigned counter = currentFunction.jumpingExprCounter;
        currentFunction.jumpingExprCounter++;

        generate_expression_ast_result(argAst);
        out("POPS %s", REG_1);
        out("LT %s %s int@0", COND_RES_VAR, REG_1);
        out("JUMPIFEQ $%s_chr%i_fail %s bool@true", currentFunction.function->name, counter, COND_RES_VAR);
        out("GT %s %s int@255", COND_RES_VAR, REG_1);
        out("JUMPIFEQ $%s_chr%i_fail %s bool@true", currentFunction.function->name, counter, COND_RES_VAR);

        out("PUSHS int@0");
        out("PUSHS %s", REG_1);
        out("INT2CHARS");
        out("JUMP $%s_chr%i_end", currentFunction.function->name, counter);

        out("LABEL $%s_chr%i_fail", currentFunction.function->name, counter);
        out("PUSHS int@1");
        out("PUSHS string@");

        out("LABEL $%s_chr%i_end", currentFunction.function->name, counter);
    }
}

void generate_internal_substr(ASTNode *argAst) {
    ASTNode *strArg = argAst->data[0].astPtr;
    ASTNode *beginIndexArg = argAst->data[1].astPtr;
    ASTNode *lenArg = argAst->data[2].astPtr;

    // Either GF@$r1 or LF@... or string@...
    char *strArgOp = NULL;
    // Either int@... or GF@$r2
    char *strLenArgOp = REG_2;

    MutableString ms;

    if (strArg->actionType == AST_CONST_STRING) {
        strArgOp = convert_to_target_string_form_cb(strArg->data[0].stringConstantValue, true);
        strLenArgOp = malloc(15);
        sprintf(strLenArgOp, "int@%u", (unsigned int) strlen(strArg->data[0].stringConstantValue));
    } else {
        if (strArg->actionType == AST_ID) {
            ms = make_var_name(strArg->data[0].symbolTableItemPtr->identifier, false);
            strArgOp = mstr_content(&ms);
        } else {
            generate_expression_ast_result(strArg);
            out("POPS %s", REG_1);
            strArgOp = REG_1;
        }

        out("STRLEN %s %s", REG_2, strArgOp);
    }

    unsigned counter = currentFunction.jumpingExprCounter;
    currentFunction.jumpingExprCounter++;

    // move begin index to REG_3
    generate_assignment_for_varname(REG_3, beginIndexArg);

    // REG_1: input string
    // REG_2: len(s)
    // REG_3: i

    // goto fail if i < 0
    out("LT %s %s int@0", COND_RES_VAR, REG_3);
    out("JUMPIFEQ $%s_substr%i_fail %s bool@true", currentFunction.function->name, counter,
        COND_RES_VAR);
    // || i > len(s)
    out("GT %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_substr%i_fail %s bool@true", currentFunction.function->name, counter,
        COND_RES_VAR);
    // || i == len(s)
    out("EQ %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_substr%i_fail %s bool@true", currentFunction.function->name, counter,
        COND_RES_VAR);

    out("CREATEFRAME");
    out("DEFVAR TF@$tmp_i_%i", counter);
    out("DEFVAR TF@$tmp_res_%i", counter);

    out("MOVE TF@$tmp_i_%i %s", counter, REG_3);
    out("MOVE TF@$tmp_res_%i string@", counter);

    // REG_3: n
    generate_assignment_for_varname(REG_3, lenArg);

    // goto fail if n < 0
    out("LT %s %s int@0", COND_RES_VAR, REG_3);
    out("JUMPIFEQ $%s_substr%i_fail %s bool@true", currentFunction.function->name, counter,
        COND_RES_VAR);

    out("ADD %s %s TF@$tmp_i_%i", REG_3, REG_3, counter);

    // REG_1: input string
    // REG_2: len(s)
    // REG_3: i + n
    // LF@$tmp_i_%i: i
    // LF@$tmp_res_%i: (empty string)

    out("GT %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_substr%i_cont %s bool@false", currentFunction.function->name, counter, COND_RES_VAR);
    // if n + i > len(s): REG_3 = lenS
    out("MOVE %s %s", REG_3, strLenArgOp);
    out("LABEL $%s_substr%i_cont", currentFunction.function->name, counter);
    // while i < REG_3: append and increment i
    out("LT %s TF@$tmp_i_%i %s", COND_RES_VAR, counter, REG_3);
    out("JUMPIFEQ $%s_substr%i_forend %s bool@false", currentFunction.function->name, counter, COND_RES_VAR);

    out("GETCHAR %s %s TF@$tmp_i_%i", REG_2, strArgOp, counter);
    out("CONCAT TF@$tmp_res_%i TF@$tmp_res_%i %s", counter, counter, REG_2);
    out("ADD TF@$tmp_i_%i TF@$tmp_i_%i int@1", counter, counter);
    out("JUMP $%s_substr%i_cont", currentFunction.function->name, counter);

    out("LABEL $%s_substr%i_forend", currentFunction.function->name, counter);
    out("PUSHS int@0");
    out("PUSHS TF@$tmp_res_%i", counter);
    out("JUMP $%s_substr%i_end", currentFunction.function->name, counter);

    out("LABEL $%s_substr%i_fail", currentFunction.function->name, counter);
    out("PUSHS int@1");
    out("PUSHS string@");
    out("LABEL $%s_substr%i_end", currentFunction.function->name, counter);

    if (strArg->actionType == AST_ID) {
        mstr_free(&ms);
    } else if (strArg->actionType == AST_CONST_STRING) {
        free(strArgOp);
        free(strLenArgOp);
    }
}

void generate_internal_ord(ASTNode *argAst) {
    ASTNode *strArg = argAst->data[0].astPtr;
    ASTNode *beginIndexArg = argAst->data[1].astPtr;

    // Either GF@$r1 or LF@... or string@...
    char *strArgOp = NULL;
    // Either int@... or GF@$r2
    char *strLenArgOp = REG_2;

    MutableString ms;

    if (strArg->actionType == AST_CONST_STRING) {
        strArgOp = convert_to_target_string_form_cb(strArg->data[0].stringConstantValue, true);
        strLenArgOp = malloc(15);
        sprintf(strLenArgOp, "int@%u", (unsigned int) strlen(strArg->data[0].stringConstantValue));
    } else {
        if (strArg->actionType == AST_ID) {
            ms = make_var_name(strArg->data[0].symbolTableItemPtr->identifier, false);
            strArgOp = mstr_content(&ms);
        } else {
            generate_expression_ast_result(strArg);
            out("POPS %s", REG_1);
            strArgOp = REG_1;
        }

        out("STRLEN %s %s", REG_2, strArgOp);
    }

    unsigned counter = currentFunction.jumpingExprCounter;
    currentFunction.jumpingExprCounter++;

    // move begin index to REG_3
    generate_assignment_for_varname(REG_3, beginIndexArg);

    // REG_1: input string
    // REG_2: len(s)
    // REG_3: i

    // goto fail if i < 0
    out("LT %s %s int@0", COND_RES_VAR, REG_3);
    out("JUMPIFEQ $%s_ord%i_fail %s bool@true", currentFunction.function->name, counter,
        COND_RES_VAR);
    // || i > len(s)
    out("GT %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_ord%i_fail %s bool@true", currentFunction.function->name, counter,
        COND_RES_VAR);
    // || i == len(s)
    out("EQ %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_ord%i_fail %s bool@true", currentFunction.function->name, counter,
        COND_RES_VAR);

    out("STRI2INT %s %s %s", REG_2, strArgOp, REG_3);
    out("PUSHS int@0");
    out("PUSHS %s", REG_2);
    out("JUMP $%s_ord%i_end", currentFunction.function->name, counter);

    out("LABEL $%s_ord%i_fail", currentFunction.function->name, counter);
    out("PUSHS int@1");
    out("PUSHS string@");
    out("LABEL $%s_ord%i_end", currentFunction.function->name, counter);

    if (strArg->actionType == AST_ID) {
        mstr_free(&ms);
    } else if (strArg->actionType == AST_CONST_STRING) {
        free(strArgOp);
        free(strLenArgOp);
    }
}
// ---------------------------------------

// Checks whether the specified AST_FUNC_CALL node leads to a call to a built-in function and calls the corresponding
// generation function if it does.
bool generate_internal_func_call(ASTNode *funcCallAst) {
    STSymbol *s = funcCallAst->left->data[0].symbolTableItemPtr;
    ASTNode *args = funcCallAst->right;

    onlyFindDefinedSymbols = true;
    if (s == symbs.print) {
        generate_print(args);
        return true;
    } else if (s == symbs.int2float) {
        generate_internal_int2float(args);
        return true;
    } else if (s == symbs.float2int) {
        generate_internal_float2int(args);
        return true;
    } else if (s == symbs.len) {
        generate_internal_len(args);
        return true;
    } else if (s == symbs.ord) {
        generate_internal_ord(args);
        return true;
    } else if (s == symbs.chr) {
        generate_internal_chr(args);
        return true;
    } else if (s == symbs.substr) {
        generate_internal_substr(args);
        return true;
    } else if (s == symbs.inputi) {
        generate_internal_inputx("int", "0");
        return true;
    } else if (s == symbs.inputb) {
        generate_internal_inputx("bool", "false");
        return true;
    } else if (s == symbs.inputs) {
        generate_internal_inputx("string", "");
        return true;
    } else if (s == symbs.inputf) {
        generate_internal_inputx("float", "0x0p+0");
        return true;
    }

    return false;
}

// Generates a function call: generates a CREATEFRAME instruction with DEFVAR instructions for the target function's arguments.
// If the function call contains other nested function calls, it first evaluates the arguments on stack, then assigns them
// into the created variables, because calling a nested function call generation would discard the current temporary frame;
// otherwise, it generates the assignments in-place, mitigating the need for additional stack operations.
void generate_func_call(ASTNode *funcCallAst) {
    // Arguments are passed in a temporary frame
    // The frame will then be pushed as LF in the function itself
    if (is_ast_empty(funcCallAst) || funcCallAst->left == NULL) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Function call has no target.\n");
        return;
    }

    STSymbol *targetFuncSymb = funcCallAst->left->data[0].symbolTableItemPtr;
    CFFunction *targetFunc = cf_get_function(targetFuncSymb->identifier, false);
    dbg("Generating func call to '%s'", targetFuncSymb->identifier);

    ASTNode *argAstList = funcCallAst->right;

    if (generate_internal_func_call(funcCallAst)) {
        onlyFindDefinedSymbols = false;
        return;
    }

    onlyFindDefinedSymbols = false;

    if (targetFunc->argumentsCount > 0) {
        if (argAstList == NULL || argAstList->dataCount != targetFunc->argumentsCount) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                           "Function call has unexpected amount of arguments.\n");
            return;
        }

        if (funcCallAst->hasInnerFuncCalls) {
            // Evaluate arguments on stack first, in last-to-first order, to make the following popping easier
            onlyFindDefinedSymbols = true;
            for (unsigned i = 0; i < argAstList->dataCount; i++) {
                ASTNode *argData = argAstList->data[argAstList->dataCount - i - 1].astPtr;

                if (argData->actionType >= AST_LOGIC && argData->actionType < AST_CONTROL) {
                    generate_logic_expression_assignment(argData, NULL);
                } else {
                    generate_expression_ast_result(argData);
                }
            }
            onlyFindDefinedSymbols = false;

            out("CREATEFRAME");
            STParam *argN = targetFuncSymb->data.func_data.params;
            while (argN != NULL) {
                MutableString varName = make_var_name(argN->id, true);

                out("DEFVAR %s", mstr_content(&varName));
                out("POPS %s", mstr_content(&varName));

                mstr_free(&varName);

                argN = argN->next;
            }
        } else {
            // No inner func calls, this is one of the inner-most calls
            // Evaluate arguments normally

            out("CREATEFRAME");
            STParam *argN = targetFuncSymb->data.func_data.params;
            ASTNodeData *argContainer = argAstList->data;

            // Setting this flag here is ok, because make_var_name doesn't perform scope lookup for TF vars
            onlyFindDefinedSymbols = true;
            while (argN != NULL) {
                MutableString varName = make_var_name(argN->id, true);

                ASTNode *argData = argContainer->astPtr;
                out("DEFVAR %s", mstr_content(&varName));
                generate_assignment_for_varname(mstr_content(&varName), argData);

                mstr_free(&varName);

                argN = argN->next;
                argContainer++;
            }
            onlyFindDefinedSymbols = false;
        }
    } else {
        out("CREATEFRAME");
    }

    out("CALL %s", targetFuncSymb->identifier);
    dbg("Function return values are on stack");

    // The return values are on stack
}

// Generates a representation of a non-logic and non-string AST node.
void generate_expression_ast(ASTNode *exprAst) {
    switch (exprAst->actionType) {
        case AST_ID:
            out_nnl("PUSHS ");
            print_var_name(exprAst);
            out_nl();
            return;
        case AST_CONST_BOOL:
        out("PUSHS bool@%s", exprAst->data[0].boolConstantValue ? "true" : "false");
            return;
        case AST_CONST_STRING:
            // This should only generate numerical expressions.
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                           "Unexpected string.\n");
            return;
        case AST_CONST_INT:
        out("PUSHS int@%li", exprAst->data[0].intConstantValue);
            return;
        case AST_CONST_FLOAT:
        out("PUSHS float@%a", exprAst->data[0].floatConstantValue);
            return;
        case AST_ADD:
        out("ADDS");
            break;
        case AST_SUBTRACT:
        out("SUBS");
            break;
        case AST_MULTIPLY:
        out("MULS");
            break;
        case AST_DIVIDE:
            if (exprAst->inheritedDataType == CF_INT) {
                out("IDIVS");
            } else {
                out("DIVS");
            }
            break;
        case AST_AR_NEGATE:
            // For NEGATE nodes converted to express a (0 - AST).
            // Left and right will have been pushed on stack, so just do the same as in SUBTRACT.
            if (exprAst->right != NULL) {
                out("SUBS");
            }
            break;
        case AST_LOG_NOT:
        case AST_LOG_AND:
        case AST_LOG_OR:
        case AST_LOG_EQ:
        case AST_LOG_NEQ:
        case AST_LOG_LT:
        case AST_LOG_GT:
        case AST_LOG_LTE:
        case AST_LOG_GTE:
            // Not gonna get here
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                           "Unexpected logic AST.\n");
            break;
        case AST_ASSIGN:
        case AST_DEFINE:
        case AST_FUNC_CALL:
        case AST_LIST:
            // Not gonna get here
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                           "Unexpected assignment, definition, func call or AST list.\n");
            break;
    }
}

// Recursively generates an AST consisting of string constants, identifiers and ADD nodes.
// It uses the most efficient way to do this based on the type of the child nodes (even though most of this
// work will have been already done by the optimiser).
// Target: 0 = push to stack, 1 = REG_1, 2 = REG_2
void generate_string_concat(ASTNode *addAst, int target) {
    ASTNode *left = addAst->left;
    ASTNode *right = addAst->right;

    if (is_direct_ast(left) && is_direct_ast(right)) {
        out_nnl("CONCAT %s ", target == 2 ? REG_2 : REG_1);
        print_var_name_or_const(left);
        out_nnl(" ");
        print_var_name_or_const(right);
        out_nl();
    } else if (is_direct_ast(left)) {
        generate_string_concat(right, 1);

        out_nnl("CONCAT %s ", target == 2 ? REG_2 : REG_1);
        print_var_name_or_const(left);
        out("%s", target == 2 ? REG_2 : REG_1);
    } else if (is_direct_ast(right)) {
        generate_string_concat(left, 1);

        out_nnl("CONCAT %s %s ", target == 2 ? REG_2 : REG_1, REG_1);
        print_var_name_or_const(right);
        out_nl();
    } else {
        generate_string_concat(left, 2);
        generate_string_concat(right, 1);

        out("CONCAT %s %s %s", target == 2 ? REG_2 : REG_1, REG_2, REG_1);
    }

    if (target == 0) {
        out("PUSHS %s", REG_1);
    }
}

// Recursively (post-order) evaluates a non-logic AST on stack. First calls itself for left and right children,
// then calls generate_expression_ast to generate a stack instruction corresponding to this node.
bool generate_expression_ast_result(ASTNode *exprAst) {
    if (exprAst == NULL) {
        dbg("Null expression");
        return false;
    }

    if (exprAst->actionType == AST_FUNC_CALL) {
        generate_func_call(exprAst);
        return true;
    }

    if (exprAst->actionType == AST_ASSIGN || exprAst->actionType == AST_DEFINE) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                       "Unexpected assignment/definition.\n");
        return false;
    }

    if (exprAst->actionType == AST_AR_NEGATE) {
        // This will probably have been done in the optimiser already
        // Checking it here isn't a problem though
        if (exprAst->left->actionType == AST_CONST_INT) {
            int i = exprAst->left->data[0].intConstantValue;
            exprAst->left->data[0].intConstantValue = -i;
        } else if (exprAst->left->actionType == AST_CONST_FLOAT) {
            double i = exprAst->left->data[0].floatConstantValue;
            exprAst->left->data[0].floatConstantValue = -i;
        } else {
            // Convert into a (0 - AST) expression
            exprAst->right = exprAst->left;
            if (exprAst->inheritedDataType == CF_INT) {
                exprAst->left = ast_leaf_consti(0);
            } else {
                exprAst->left = ast_leaf_constf(0.0);
            }
        }
    }

    if (exprAst->inheritedDataType == CF_STRING) {
        if (exprAst->actionType == AST_ADD) {
            generate_string_concat(exprAst, 0);
        } else if (exprAst->actionType == AST_ID) {
            out_nnl("PUSHS ");
            print_var_name(exprAst);
            out_nl();
        } else if (exprAst->actionType == AST_CONST_STRING) {
            char *s = convert_to_target_string_form(exprAst->data[0].stringConstantValue);
            out("PUSHS string@%s", s);
            free(s);
        } else {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                           "Unexpected operation for strings.\n");
            return false;
        }

        return true;
    }

    if (exprAst->left != NULL)
        generate_expression_ast_result(exprAst->left);
    if (exprAst->right != NULL)
        generate_expression_ast_result(exprAst->right);

    generate_expression_ast(exprAst);
    return true;
}

// Creates a string with a unique name for a label that is generated as a part of jumping logic expression tree
// evaluation. This is done simply using a static counter, which can be global for the whole program.
char *make_next_logic_label() {
    static unsigned counter = 0;
    char *a = malloc(UINT_DIGITS);
    sprintf(a, "$$log_%u", counter++);
    return a;
}

// Evaluates a simple logic expression (comparison, constant, identifier or function call). Generates a jump
// to *trueLabel when the result is true, and a jump to *falseLabel when it's false.
bool generate_simple_logic_expression(ASTNode *exprAst, char *trueLabel, char *falseLabel) {
    ASTNode *left = exprAst->left;
    ASTNode *right = exprAst->right;
    ASTNodeType t = exprAst->actionType;

    if (exprAst->actionType == AST_CONST_BOOL) {
        if (exprAst->data[0].boolConstantValue) {
            out("JUMP %s", trueLabel);
        } else {
            out("JUMP %s", falseLabel);
        }

        return true;
    }

    if (exprAst->actionType == AST_ID) {
        out_nnl("JUMPIFEQ %s ", trueLabel);
        print_var_name(exprAst);
        out(" bool@true");
        out("JUMP %s", falseLabel);
        return true;
    }

    if (exprAst->actionType == AST_FUNC_CALL) {
        if (exprAst->inheritedDataType != CF_BOOL) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                           "Unexpected call to '%s' in a logic expression.\n",
                           exprAst->left->data[0].symbolTableItemPtr->identifier);
            return false;
        }

        generate_func_call(exprAst);
        // One bool return value on stack
        out("PUSHS bool@true");
        out("JUMPIFEQS %s", trueLabel);
        out("JUMP %s", falseLabel);
        return true;
    }

    if (is_direct_ast(left) && is_direct_ast(right)) {
        if (t == AST_LOG_EQ || t == AST_LOG_NEQ) {
            out_nnl("%s %s ", t == AST_LOG_EQ ? "JUMPIFEQ" : "JUMPIFNEQ", trueLabel);
            print_var_name_or_const(left);
            out_nnl(" ");
            print_var_name_or_const(right);
            out_nl();
        } else if (t == AST_LOG_LT || t == AST_LOG_GT) {
            out_nnl("%s %s ", t == AST_LOG_LT ? "LT" : "GT", COND_RES_VAR);
            print_var_name_or_const(left);
            out_nnl(" ");
            print_var_name_or_const(right);
            out_nl();
            out("JUMPIFEQ %s bool@true %s", trueLabel, COND_RES_VAR);

        } else if (t == AST_LOG_LTE || t == AST_LOG_GTE) {
            out_nnl("JUMPIFEQ %s ", trueLabel);
            print_var_name_or_const(left);
            out_nnl(" ");
            print_var_name_or_const(right);
            out_nl();

            out_nnl("%s %s ", t == AST_LOG_LTE ? "LT" : "GT", COND_RES_VAR);
            print_var_name_or_const(left);
            out_nnl(" ");
            print_var_name_or_const(right);
            out_nl();

            out("JUMPIFEQ %s bool@true %s", trueLabel, COND_RES_VAR);
        } else { // Sanity check
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                           "Malformed logical expression.\n");
            return false;
        }

        out("JUMP %s", falseLabel);
        return true;
    } else {
        if (left->inheritedDataType == CF_BOOL && right->inheritedDataType == CF_BOOL) {
            generate_logic_expression_assignment(left, NULL);
            generate_logic_expression_assignment(right, NULL);
        } else if (left->inheritedDataType == CF_BOOL || right->inheritedDataType == CF_BOOL) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                           "Unexpected value in logical expression.\n");
            return false;
        } else {
            generate_expression_ast_result(left);
            generate_expression_ast_result(right);
        }

        if (t == AST_LOG_EQ) {
            out("EQS");
            out("PUSHS bool@true");
            out("JUMPIFEQS %s", trueLabel);
        } else if (t == AST_LOG_NEQ) {
            out("EQS");
            out("PUSHS bool@false");
            out("JUMPIFEQS %s", trueLabel);
        } else if (t == AST_LOG_LT) {
            out("LTS");
            out("PUSHS bool@true");
            out("JUMPIFEQS %s", trueLabel);
        } else if (t == AST_LOG_GT) {
            out("GTS");
            out("PUSHS bool@true");
            out("JUMPIFEQS %s", trueLabel);
        } else if (t == AST_LOG_LTE || t == AST_LOG_GTE) {
            out("POPS %s", COND_RHS_VAR);
            out("POPS %s", COND_LHS_VAR);
            out("JUMPIFEQ %s %s %s", trueLabel, COND_LHS_VAR, COND_RHS_VAR);

            out("%s %s %s %s", t == AST_LOG_LTE ? "LT" : "GT", COND_RES_VAR, COND_LHS_VAR, COND_RHS_VAR);
            out("JUMPIFEQ %s bool@true %s", trueLabel, COND_RES_VAR);
        } else { // Sanity check
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                           "Malformed logical expression.\n");
            return false;
        }

        out("JUMP %s", falseLabel);
        return true;
    }
}

// Recursively evaluates a jumping logic expression tree by calling itself when the *exprAst is an AND, OR or NOT
// and creating and/or propagating the true and false labels. The evaluation of the simple logical operations
// (e.g. equals, less than) themselves is done in generate_simple_logic_expression.
bool generate_logic_expression_tree(ASTNode *exprAst, char *trueLabel, char *falseLabel) {
    ASTNode *left = exprAst->left;
    ASTNode *right = exprAst->right;

    bool result = true;
    if (exprAst->actionType == AST_LOG_AND) {
        char *next = make_next_logic_label();

        result &= generate_logic_expression_tree(left, next, falseLabel);
        out("LABEL %s", next);
        result &= generate_logic_expression_tree(right, trueLabel, falseLabel);

        free(next);
    } else if (exprAst->actionType == AST_LOG_OR) {
        char *next = make_next_logic_label();

        result &= generate_logic_expression_tree(left, trueLabel, next);
        out("LABEL %s", next);
        result &= generate_logic_expression_tree(right, trueLabel, falseLabel);

        free(next);
    } else if (exprAst->actionType == AST_LOG_NOT) {
        return generate_logic_expression_tree(left, falseLabel, trueLabel);
    } else {
        return generate_simple_logic_expression(exprAst, trueLabel, falseLabel);
    }

    return result;
}

// Evaluates the *exprAst using generate_logic_expression_tree and generates true/false labels that
// assign true or false into the *targetVarName variable.
// *targetVarName must contain a target variable name (e.g. LF@var).
// When *varName is NULL, the expression result will be pushed on stack.
// Increments the jumping expression counter.
bool generate_logic_expression_assignment(ASTNode *exprAst, const char *targetVarName) {
    unsigned counter = currentFunction.jumpingExprCounter;
    currentFunction.jumpingExprCounter++;

    char i[UINT_DIGITS];
    sprintf(i, "%u", counter);

    MutableString trueLabelStr, falseLabelStr;
    mstr_make(&trueLabelStr, 5, "$", currentFunction.function->name, "_", i, "_true");
    mstr_make(&falseLabelStr, 5, "$", currentFunction.function->name, "_", i, "_false");

    generate_logic_expression_tree(exprAst, mstr_content(&trueLabelStr), mstr_content(&falseLabelStr));
    out("LABEL %s", mstr_content(&trueLabelStr));
    if (targetVarName == NULL) {
        out("PUSHS bool@true");
    } else {
        out("MOVE %s bool@true", targetVarName);
    }

    out("JUMP $%s_%i_end", currentFunction.function->name, counter);

    out("LABEL %s", mstr_content(&falseLabelStr));
    if (targetVarName == NULL) {
        out("PUSHS bool@false");
    } else {
        out("MOVE %s bool@false", targetVarName);
    }

    out("LABEL $%s_%i_end", currentFunction.function->name, counter);

    mstr_free(&trueLabelStr);
    mstr_free(&falseLabelStr);

    return true;
}

// Evaluates the *value AST and generates an instruction to move the result into variable *varName.
// *varName must contain a target variable name (e.g. LF@var).
// When *varName is NULL, generates an assignment into REG_1 (GF@$r1).
// Optimisation: when the *value AST is a constant or an ID, generates a MOVE instead of evaluating the constant on stack.
void generate_assignment_for_varname(const char *varName, ASTNode *value) {
    if (varName == NULL) {
        if (value->actionType >= AST_LOGIC && value->actionType < AST_CONTROL) {
            generate_logic_expression_assignment(value, REG_1);
        } else {
            // The right side is not a CONST or ID, evaluate expression on stack and pop it.
            generate_expression_ast_result(value);
            out("POPS %s", REG_1);
        }

        return;
    }

    switch (value->actionType) {
        // If the right side is a CONST or ID, generate a simple MOVE
        case AST_CONST_INT:
        out("MOVE %s int@%li", varName, value->data[0].intConstantValue);
            break;
        case AST_CONST_FLOAT:
        out("MOVE %s float@%a", varName, value->data[0].floatConstantValue);
            break;
        case AST_CONST_BOOL:
        out("MOVE %s bool@%s", varName, value->data[0].boolConstantValue ? "true" : "false");
            break;
        case AST_CONST_STRING: {
            char *s = convert_to_target_string_form(value->data[0].stringConstantValue);
            out("MOVE %s string@%s", varName, s);
            free(s);
        }
            break;
        case AST_ID:
            out_nnl("MOVE %s ", varName);
            print_var_name(value);
            out_nl();
            break;
        default:
            if (value->actionType >= AST_LOGIC && value->actionType < AST_CONTROL) {
                generate_logic_expression_assignment(value, varName);
            } else {
                // The right side is not a CONST or ID, evaluate expression on stack and pop it.
                generate_expression_ast_result(value);
                out("POPS %s", varName);
            }
            break;
    }
}

void generate_assignment(ASTNode *asgAst);

// Generates a multi-assignment of expanded function call return values
// (AST_LIST of AST_IDs (:)= AST_LIST with one AST_FUNC_CALL in its data).
void generate_assignment_with_function_expansion(ASTNode *asgAst) {
    STSymbol *funcSymb = asgAst->right->data[0].astPtr->left->data[0].symbolTableItemPtr;

    if (asgAst->left->dataCount != funcSymb->data.func_data.ret_types_count) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                       "Assignment left-hand side variables don't match the right-hand side function's return values.\n");
        return;
    }

    generate_func_call(asgAst->right->data[0].astPtr);

    for (unsigned i = 0; i < asgAst->left->dataCount; i++) {
        // Generate the pops left-to-right (the left-most variable corresponds to the first return value, which
        // will be at the top of stack, because return values are pushed last-to-first).
        if (asgAst->left->data[i].astPtr->inheritedDataType == CF_BLACK_HOLE
            || asgAst->left->data[i].astPtr->data[0].symbolTableItemPtr->reference_counter == 0) {
            out("POPS %s", REG_1);
            continue;
        }

        MutableString varName = make_var_name(
                asgAst->left->data[i].astPtr->data[0].symbolTableItemPtr->identifier, false);
        out("POPS %s", mstr_content(&varName));
        asgAst->left->data[i].astPtr->data[0].symbolTableItemPtr->data.var_data.defined = true;
        mstr_free(&varName);
    }
}

// Generates a multi-assignment (AST_LIST of AST_IDs (:)= AST_LIST of values).
// It first evaluates the right-hand side on stack (left-to-right), then it generates POPS for the respective values.
// If there's a CF_BLACK_HOLE on the left-hand side, it generates a POPS to REG_1. The same is performed when a variable
// is found on the left-hand side multiple times; then, only the right-most assignment to this variable is performed.
void generate_multi_assignment(ASTNode *asgAst) {
    if (asgAst->right->actionType != AST_LIST) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Expected AST_LIST on the right side, got %i instead.\n", asgAst->right->actionType);
    }

    if (asgAst->left->dataCount != asgAst->right->dataCount) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                       "Assignment left-hand side variable count doesn't match the right-hand side variable count.\n");
        return;
    }

    if (asgAst->left->dataCount == 1) {
        ASTNode *tmpAssignNode = ast_node(AST_ASSIGN);
        ASTNode *tmpAssignLeft = asgAst->left->data[0].astPtr;
        tmpAssignNode->left = tmpAssignLeft;
        tmpAssignNode->right = asgAst->right->data[0].astPtr;
        generate_assignment(tmpAssignNode);
        free(tmpAssignNode);
    } else {
        for (unsigned i = 0; i < asgAst->left->dataCount; i++) {
            ASTNode *valNode = asgAst->right->data[i].astPtr;
            ASTNode *idNode = asgAst->left->data[i].astPtr;

            if (!valNode->hasInnerFuncCalls && valNode->actionType != AST_FUNC_CALL
                && (idNode->inheritedDataType == CF_BLACK_HOLE
                    || idNode->data[0].symbolTableItemPtr->reference_counter == 0)) {

                // If the variable that is being assigned to has no references or is a throwaway
                // and the expression contains no function calls, we can safely omit its generation
                continue;
            }

            if (valNode->actionType >= AST_LOGIC && valNode->actionType < AST_CONTROL) {
                generate_logic_expression_assignment(valNode, NULL);
            } else {
                generate_expression_ast_result(valNode);
            }
        }

        for (unsigned i = 0; i < asgAst->left->dataCount; i++) {
            unsigned currentVarIndex = asgAst->left->dataCount - i - 1;
            ASTNode *idNode = asgAst->left->data[currentVarIndex].astPtr;
            ASTNode *valNode = asgAst->right->data[currentVarIndex].astPtr;

            if (idNode->inheritedDataType == CF_BLACK_HOLE
                || idNode->data[0].symbolTableItemPtr->reference_counter == 0) {

                // Expressions with function calls have been evaluated, pop them into a throwaway variable
                if (valNode->hasInnerFuncCalls || valNode->actionType == AST_FUNC_CALL) {
                    out("POPS %s", REG_1);
                }
            } else {
                // Check whether there's the same variable in the assignment list that's to the right
                // side of this one. If so, throw the result away.

                bool hadLeft = false;
                for (unsigned j = currentVarIndex + 1; j < asgAst->left->dataCount; j++) {
                    if (idNode->data[0].symbolTableItemPtr ==
                        asgAst->left->data[j].astPtr->data[0].symbolTableItemPtr) {
                        out("POPS %s", REG_1);
                        hadLeft = true;
                        break;
                    }
                }

                if (!hadLeft) {
                    out_nnl("POPS ");
                    print_var_name(idNode);
                    out_nl();

                    idNode->data[0].symbolTableItemPtr->data.var_data.defined = true;
                }
            }
        }
    }
}

// Entry point for generation of an assignment or a definition. If the left child is an AST_LIST,
// a multi-assignment will be generated. If the left child is an AST_LIST and the right child is a list
// with only one AST_FUNC_CALL child, a multi-assignment of expanded function return values will be generated.
// If the left child is an AST_ID, generate_assignment_for_varname will be called that handles the evaluation.
void generate_assignment(ASTNode *asgAst) {
    if (asgAst->left->actionType == AST_LIST) {
        if (asgAst->right->actionType == AST_LIST && asgAst->right->dataCount == 1
            && asgAst->right->data[0].astPtr->actionType == AST_FUNC_CALL) {
            generate_assignment_with_function_expansion(asgAst);
        } else {
            generate_multi_assignment(asgAst);
        }
        return;
    }

    if (asgAst->left->actionType != AST_ID) { // Sanity check
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Invalid assignment.\n");
        return;
    } else if (asgAst->left->inheritedDataType == CF_BLACK_HOLE
               || asgAst->left->data[0].symbolTableItemPtr->reference_counter == 0) {

        if (asgAst->right->hasInnerFuncCalls || asgAst->right->actionType == AST_FUNC_CALL) {
            // Inner func call may have side effects, evaluate the expression and throw the result away
            generate_assignment_for_varname(NULL, asgAst->right);
        }
    } else {
        MutableString varName = make_var_name(asgAst->left->data[0].symbolTableItemPtr->identifier, false);
        onlyFindDefinedSymbols = true;
        generate_assignment_for_varname(mstr_content(&varName), asgAst->right);
        onlyFindDefinedSymbols = false;
        asgAst->left->data[0].symbolTableItemPtr->data.var_data.defined = true;
        mstr_free(&varName);
    }
}

// Generates a return statement. All CF_RETURN statements must have an AST_LIST as their body,
// passing NULL is considered equal to passing an AST_LIST with no data. (This is used when generating implict returns.)
void generate_return_statement(ASTNode *retAstList) {
    // If we're generating main and we're not generating it as a function,
    // end it with an EXIT instruction. Otherwise, generate it normally.
    if (currentFunction.isMain && !currentFunction.generateMainAsFunction) {
        if (retAstList != NULL && retAstList->dataCount != 0) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Expected an empty return statement for function 'main'.\n");
            return;
        }

        out("EXIT int@0");
        currentFunction.terminated = true;
        return;
    }

    bool hasNamedReturnValues = currentFunction.function->returnValues != NULL
                                && currentFunction.function->returnValues->variable.name != NULL;

    if (currentFunction.function->returnValuesCount == 0) {
        // Function should have no return values, this statement has some values
        if (retAstList != NULL && retAstList->dataCount > 0) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Expected an empty return statement for function '%s'.\n",
                           currentFunction.function->name);
            return;
        }
    } else {
        // Function has unnamed return values -> it must have a non-empty return AST_LIST
        if (!hasNamedReturnValues && (retAstList == NULL || retAstList->actionType != AST_LIST)) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                           "Unexpected AST in RETURN statement.\n");
            return;
        }

        // Function has named return values -> the return statement can have values, but in that case,
        // it must cover all of the return values
        if (hasNamedReturnValues && retAstList != NULL && retAstList->dataCount != 0
            && retAstList->dataCount != currentFunction.function->returnValuesCount) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Return statement of the function '%s' with named return values should either explicitly specify all return values or contain none.\n",
                           currentFunction.function->name);
            return;
        }

        // Function has unnamed return values -> the statement must have a matching number of return values
        if (!hasNamedReturnValues && retAstList->dataCount != currentFunction.function->returnValuesCount) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Return statement data count doesn't match function's '%s' return values count.\n",
                           currentFunction.function->name);
            return;
        }
    }

    CFVarListNode *argNode = currentFunction.function->returnValues;
    if (retAstList != NULL && retAstList->dataCount > 0) {
        // Evaluate return values ASTs on stack
        // The first return value will be generated last (it will be on top of stack)

        for (unsigned i = 0; i < retAstList->dataCount; i++) {
            ASTNode *ast = retAstList->data[retAstList->dataCount - i - 1].astPtr;

            if (!ast_infer_node_type(ast)) {
                stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                               "Types of values in return statement of function '%s' couldn't be inferred.\n",
                               currentFunction.function->name);
                return;
            }

            if (ast->inheritedDataType != argNode->variable.dataType) {
                stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                               "Function '%s': Invalid return value type (return value on index %u).\n",
                               currentFunction.function->name, retAstList->dataCount - i - 1);
                return;
            }

            if (ast->actionType >= AST_LOGIC && ast->actionType < AST_CONTROL) {
                generate_logic_expression_assignment(ast, NULL);
            } else {
                generate_expression_ast_result(ast);
            }

            argNode = argNode->next;
        }
    } else {
        // The function has named return values, so they're local variables -> push their values on stack
        for (unsigned i = 0; i < currentFunction.function->returnValuesCount; i++) {
            out_nnl("PUSHS ");
            print_var_name_id(argNode->variable.name);
            out_nl();

            argNode = argNode->next;
        }
    }

    // Delete the local frame
    out("POPFRAME");
    out("RETURN");

    // Set the flags signalising that current function has had a return statement and whether
    // it's been found in a branched statement. If a return statement has been found before in a branch
    // and this one is not, set the terminatedInBranch flag back to false. This is used to determine
    // whether an implicit return should be generated at the end of the function (to make sure the interpret
    // will always return from the function).
    if (!currentFunction.terminated) {
        currentFunction.terminated = true;
        currentFunction.terminatedInBranch = currentFunction.isInBranch;
    } else {
        if (currentFunction.terminatedInBranch && !currentFunction.isInBranch) {
            currentFunction.terminatedInBranch = false;
        }
    }
}

// Generates a statement of CF_IF type, recursively generates the bodies of its THEN and ELSE blocks.
// Increases the current function's IF-counter.
void generate_if_statement(CFStatement *stat) {
    dbg("Generating if statement #%i", currentFunction.ifCounter);

    unsigned counter = currentFunction.ifCounter;
    currentFunction.ifCounter++;

    ast_infer_node_type(stat->data.ifData->conditionalAst);
    if (stat->data.ifData->conditionalAst->inheritedDataType != CF_BOOL) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                       "Unexpected non-logical expression in if statement.\n");
        return;
    }

    bool hasElse = !is_statement_empty(stat->data.ifData->elseStatement);

    // Make the names of the then/else labels
    char i[UINT_DIGITS];
    sprintf(i, "%u", counter);

    MutableString trueLabelStr, falseLabelStr;

    mstr_make(&trueLabelStr, 5, "$", stat->parentFunction->name, "_if", i, "_then");
    // If this IF statement doesn't have an ELSE block, jump directly to its end when the conditional expression is false
    mstr_make(&falseLabelStr, 5, "$", stat->parentFunction->name, "_if", i, hasElse ? "_else" : "_end");

    generate_logic_expression_tree(stat->data.ifData->conditionalAst, mstr_content(&trueLabelStr),
                                   mstr_content(&falseLabelStr));

    // Backup the current state of the flag specifying whether we're inside of a branch
    // and set it to true. (Used in return statements.)
    bool originalBranchState = currentFunction.isInBranch;
    currentFunction.isInBranch = true;
    out("LABEL %s", mstr_content(&trueLabelStr));

    // Push the THEN statement's symbol table, generate the statement (recursively) and pop the table.
    symtable_stack_push(&currentFunction.stStack, stat->data.ifData->thenStatement->localSymbolTable);
    generate_statement(stat->data.ifData->thenStatement);
    symtable_stack_pop(&currentFunction.stStack);

    if (hasElse) {
        out("JUMP $%s_if%i_end", stat->parentFunction->name, counter);
        out("LABEL %s", mstr_content(&falseLabelStr));
        symtable_stack_push(&currentFunction.stStack, stat->data.ifData->elseStatement->localSymbolTable);
        generate_statement(stat->data.ifData->elseStatement);
        symtable_stack_pop(&currentFunction.stStack);
    }

    currentFunction.isInBranch = originalBranchState;

    out("LABEL $%s_if%i_end", stat->parentFunction->name, counter);

    mstr_free(&trueLabelStr);
    mstr_free(&falseLabelStr);
    dbg("Finished if #%i", counter);
}

// Generates a statement of CF_FOR type, recursively generates its body. Increases the current function's IF-counter.
void generate_for_statement(CFStatement *stat) {
    dbg("Generating for statement (if #%i)", currentFunction.ifCounter);

    // FORs have a special symtable for their header, push it
    symtable_stack_push(&currentFunction.stStack, stat->localSymbolTable);

    unsigned counter = currentFunction.ifCounter;
    currentFunction.ifCounter++;

    if (stat->data.forData->definitionAst != NULL) {
        ast_infer_node_type(stat->data.forData->definitionAst);
        generate_assignment(stat->data.forData->definitionAst);
    }

    out("LABEL $%s_for%i_begin", stat->parentFunction->name, counter);

    if (stat->data.forData->conditionalAst != NULL) {
        ast_infer_node_type(stat->data.forData->conditionalAst);
        if (stat->data.forData->conditionalAst->inheritedDataType != CF_BOOL) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                           "Unexpected non-logical expression in for definition.\n");
            return;
        }

        char i[UINT_DIGITS];
        sprintf(i, "%u", counter);

        MutableString trueLabelStr, falseLabelStr;

        mstr_make(&trueLabelStr, 5, "$", stat->parentFunction->name, "_for", i, "_then");
        mstr_make(&falseLabelStr, 5, "$", stat->parentFunction->name, "_for", i, "_end");

        generate_logic_expression_tree(stat->data.forData->conditionalAst, mstr_content(&trueLabelStr),
                                       mstr_content(&falseLabelStr));

        out("LABEL %s", mstr_content(&trueLabelStr));
        mstr_free(&trueLabelStr);
        mstr_free(&falseLabelStr);
    }

    // Backup the current state of the flag specifying whether we're inside of a branch
    // and set it to true. (Used in return statements.)
    bool originalBranchState = currentFunction.isInBranch;
    currentFunction.isInBranch = true;

    // The FOR body has another symbol table, push it.
    symtable_stack_push(&currentFunction.stStack, stat->data.forData->bodyStatement->localSymbolTable);
    generate_statement(stat->data.forData->bodyStatement);
    symtable_stack_pop(&currentFunction.stStack);
    currentFunction.isInBranch = originalBranchState;

    if (stat->data.forData->afterthoughtAst != NULL) {
        ast_infer_node_type(stat->data.forData->afterthoughtAst);
        generate_assignment(stat->data.forData->afterthoughtAst);
    }
    symtable_stack_pop(&currentFunction.stStack);

    out("JUMP $%s_for%i_begin", stat->parentFunction->name, counter);
    out("LABEL $%s_for%i_end", stat->parentFunction->name, counter);

    dbg("Finished for (if #%i)", counter);
}

// Generates a statement of CF_BASIC type. Checks the type of the statement's body AST: basic statements
// must always be function calls, variable definitions or assignments.
void generate_basic_statement(CFStatement *stat) {
    if (stat->data.bodyAst == NULL) return;
    ast_infer_node_type(stat->data.bodyAst);

    switch (stat->data.bodyAst->actionType) {
        case AST_FUNC_CALL:
            if (stat->data.bodyAst->left->data[0].symbolTableItemPtr->data.func_data.ret_types_count > 0) {
                stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                               "Unexpected call outside an assigment or an expression to a function that returns values.\n");
                return;
            }

            generate_func_call(stat->data.bodyAst);
            break;
        case AST_DEFINE:
        case AST_ASSIGN:
            generate_assignment(stat->data.bodyAst);
            break;
        default: // sanity check, shouldn't get here
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                           "Invalid top-level statement, expected definition, assignment or function call.\n");
            return;
    }
}

// Entry point for generation of a statement. Recursively generates the specified statement and its following statements.
void generate_statement(CFStatement *stat) {
    if (is_statement_empty(stat)) {
        if (stat != NULL && stat->followingStatement != NULL && stat->followingStatement->statementType != CF_IF) {
            dbg("Omitting empty statement");
        }
    } else {
        if (compiler_result != COMPILER_RESULT_SUCCESS) {
            out("# Code generation error occurred; omitting the rest.");
            stderr_message("codegen", ERROR, compiler_result, "Code generation error occurred; omitting the rest.\n");
            return;
        }

        switch (stat->statementType) {
            case CF_BASIC:
                generate_basic_statement(stat);
                break;
            case CF_IF:
                generate_if_statement(stat);
                break;
            case CF_FOR:
                generate_for_statement(stat);
                break;
            case CF_RETURN:
                generate_return_statement(stat->data.bodyAst);
                break;
        }
    }

    if (stat->followingStatement != NULL) {
        generate_statement(stat->followingStatement);
    }
}

// Recursively assigns unique number to all scopes (symbol tables) in the current function
// and generates DEFVAR instructions for all local variables in all found scopes.
void generate_definitions(CFStatement *stat) {
    if (stat == NULL) return;

    if (stat->localSymbolTable->symbol_prefix == 0) {
        stat->localSymbolTable->symbol_prefix = currentFunction.scopeCounter++;

        symtable_stack_push(&currentFunction.stStack, stat->localSymbolTable);
        for (unsigned ai = 0; ai < stat->localSymbolTable->arr_size; ai++) {
            STItem *it = stat->localSymbolTable->arr[ai];
            while (it != NULL) {
                STSymbol *symb = &it->data;

                if (symb->reference_counter > 0 && symb->type == ST_SYMBOL_VAR) {
                    if (!symb->data.var_data.is_argument_variable) {
                        MutableString varName = make_var_name(symb->identifier, false);
                        char *varNameP = mstr_content(&varName);
                        out("DEFVAR %s", varNameP);

                        if (symb->data.var_data.is_return_val_variable) {
                            switch (symb->data.var_data.type) {
                                case CF_INT:
                                out("MOVE %s int@0", varNameP);
                                    break;
                                case CF_FLOAT:
                                out("MOVE %s float@%a", varNameP, 0.0);
                                    break;
                                case CF_STRING:
                                out("MOVE %s string@", varNameP);
                                    break;
                                case CF_BOOL:
                                out("MOVE %s bool@false", varNameP);
                                    break;
                                default:
                                    stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                                                   "Unexpected return value '%s' type.\n", symb->identifier);
                                    break;
                            }
                        }

                        mstr_free(&varName);
                    }
                }

                it = it->next;
            }
        }
        symtable_stack_pop(&currentFunction.stStack);
    }

    if (stat->statementType == CF_IF) {
        generate_definitions(stat->data.ifData->thenStatement);
        generate_definitions(stat->data.ifData->elseStatement);
    } else if (stat->statementType == CF_FOR) {
        generate_definitions(stat->data.forData->bodyStatement);
    }

    if (stat->followingStatement != NULL) {
        generate_definitions(stat->followingStatement);
    }
}

// Generates a function.
void generate_function(CFFunction *fun) {
    dbg("Function '%s'", fun->name);

    struct cfgraph_program_structure *prog = get_program();
    STItem *funcSymbol = symtable_find(prog->globalSymtable, fun->name);
    if (funcSymbol->data.reference_counter == 0) {
        dbg("Function not used");
        stderr_message("codegen", WARNING, COMPILER_RESULT_SUCCESS, "Function '%s' is not used anywhere.\n",
                       fun->name);
        // An unused function generation may not be omitted, because codegen might be the on running certain semantic checks (if optimisation is off).
        // return;
    }

    if (is_statement_empty(fun->rootStatement)) {
        stderr_message("codegen", WARNING, COMPILER_RESULT_SUCCESS, "Function '%s' is empty.\n", fun->name);

        if (fun->returnValuesCount > 0 && fun->returnValues->variable.name == NULL) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Empty function with parameters is missing a return statement.\n");
            return;
        }

        out("LABEL %s", fun->name);
        dbg("Function empty");
        out("RETURN");
        return;
    }

    currentFunction.function = fun;
    currentFunction.scopeCounter = 1;
    currentFunction.jumpingExprCounter = 0;
    currentFunction.ifCounter = 0;
    currentFunction.isInBranch = false;
    currentFunction.terminatedInBranch = false;
    currentFunction.terminated = false;

    symtable_stack_push(&currentFunction.stStack, fun->symbolTable);

    out("LABEL %s", fun->name);

    // In case of typical functions, a TF with local variables would be created before the call.
    // Function main must make its own frame, if it's not being generated as a function.
    if (currentFunction.isMain && !currentFunction.generateMainAsFunction) {
        out("CREATEFRAME");
    }

    out("PUSHFRAME");

    generate_definitions(fun->rootStatement);
    generate_statement(fun->rootStatement);

    // Return from the function will be generated from the first RETURN statement.
    // If it hasn't been generated, we must generate it explicitly.
    if (!currentFunction.terminated) {
        if (fun->returnValuesCount == 0 || fun->returnValues->variable.name != NULL) {
            generate_return_statement(NULL);
        } else {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Function '%s' is missing a return statement.\n", fun->name);
            return;
        }
    }

    // If all return statements were found in branches (inside IFs or FORs), the terminated flag will be set to true,
    // but there would still not be a return statement generated at the end of the function and the interpret would
    // continue executing the following function. To prevent this, generate an implicit return statement with default
    // return values. This might be redundant in many cases and might be improved significantly by implementing a better
    // static analysis of code branches.
    if (currentFunction.terminatedInBranch) {
        if (fun->returnValuesCount != 0 && fun->returnValues->variable.name == NULL) {
            stderr_message("codegen", WARNING, COMPILER_RESULT_SUCCESS,
                           "Function '%s' has no return statements outside branches. Generated a return statement with default values.\n",
                           fun->name);

            CFVarListNode *n = fun->returnValues;
            while (n != NULL) {
                switch (n->variable.dataType) {
                    case CF_BOOL:
                    out("PUSHS bool@false");
                        break;
                    case CF_INT:
                    out("PUSHS int@0");
                        break;
                    case CF_FLOAT:
                    out("PUSHS float@0x0p+0");
                        break;
                    case CF_STRING:
                    out("PUSHS string@");
                        break;
                    default:
                        break;
                }
                n = n->next;
            }
        }

        out("POPFRAME");
        out("RETURN");
    }

    dbg("Function '%s' end", fun->name);
}

void tcg_generate() {
    if (cf_error) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Target code generator called on an erroneous CFG (error code %i).\n", cf_error);
        return;
    }

    if (compiler_result != COMPILER_RESULT_SUCCESS) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Target code generator called with an erroneous compiler state.\n", cf_error);
        return;
    }

    // During code generation, all data types must be resolved correctly and unequivocally
    ast_set_strict_inference_state(true);

    CFProgram *prog = get_program();
    CFFuncListNode *n = prog->functionList;

    STItem *mainSym = symtable_find(prog->globalSymtable, "main");
    if (prog->mainFunc == NULL || mainSym == NULL) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE,
                       "Undefined main function.\n");
        return;
    }

    // When main is called from the code, it must be generated as a function (ended using POPFRAME/RETURN);
    // otherwise, it can be ended using EXIT directly.
    bool generateMainAsFunc = mainSym->data.reference_counter > 1;

    out(".IFJcode20");

    out("DEFVAR %s", COND_RES_VAR);
    out("DEFVAR %s", COND_LHS_VAR);
    out("DEFVAR %s", COND_RHS_VAR);
    out("DEFVAR %s", REG_1);
    out("DEFVAR %s", REG_2);

    find_internal_symbols(prog->globalSymtable);

    if (symbs.reg3Used) {
        out("DEFVAR %s", REG_3);
    }

    if (is_statement_empty(prog->mainFunc->rootStatement)) {
        stderr_message("codegen", WARNING, COMPILER_RESULT_SUCCESS, "Empty main function.\n");
        out("EXIT int@0");
    }

    if (generateMainAsFunc) {
        out("CREATEFRAME");
        out("CALL main");
        out("EXIT int@0");
    } else if (&n->fun != prog->mainFunc) {
        out("JUMP main");
    }

    while (n != NULL) {
        currentFunction.isMain = &n->fun == prog->mainFunc;
        currentFunction.generateMainAsFunction = generateMainAsFunc;

        generate_function(&n->fun);

        // This should alawys only pop one ST, the function's top-level one
        while (currentFunction.stStack.top != NULL) {
            symtable_stack_pop(&currentFunction.stStack);
        }

        n = n->next;
    }
}
