/** @file code_generator.c
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

#define TCG_DEBUG 1

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

#define PRINT_VAR "GF@$print"

struct {
    CFFunction *function;
    unsigned scopeCounter;
    unsigned jumpingExprCounter;
    unsigned ifCounter;

    bool isMain;
    bool generateMainAsFunction;
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

    bool printUsed;
    bool reg3Used;
} symbs;

#define find_internal_symbol(symbol) (it = symtable_find(globSt, (symbol))) == NULL ? NULL : (&it->data)

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

    symbs.printUsed =
            symbs.print != NULL && symbs.print->reference_counter > 0;
    symbs.reg3Used =
            (symbs.ord != NULL && symbs.ord->reference_counter > 0)
            || (symbs.substr != NULL && symbs.substr->reference_counter > 0);
}

bool is_ast_empty(ASTNode *ast) {
    return ast == NULL || (ast->left == NULL && ast->right == NULL && ast->dataCount == 0);
}

bool is_statement_empty(CFStatement *stat) {
    if (stat == NULL) return true;

    switch (stat->statementType) {
        case CF_BASIC:
            if (stat->parentStatement != NULL && (stat->parentStatement->statementType == CF_IF
                                                  || stat->parentStatement->statementType == CF_FOR)) {
                return false;
            }

            return is_ast_empty(stat->data.bodyAst);
        case CF_IF:
            return is_ast_empty(stat->data.ifData->conditionalAst) ||
                   (is_statement_empty(stat->data.ifData->thenStatement) &&
                    is_statement_empty(stat->data.ifData->elseStatement));
        case CF_FOR:
            return is_statement_empty(stat->data.forData->bodyStatement);
        case CF_RETURN:
            for (unsigned i = 0; i < stat->data.bodyAst->dataCount; i++) {
                if (is_ast_empty(stat->data.bodyAst->data[i].astPtr)) return true;
            }

            return false;
        default:
            return true;
    }
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

bool onlyFindDefinedSymbols = false;

SymbolTable *find_sym_table(const char *id, CFStatement *stat) {
    while (stat != NULL) {
        STItem *it;

        if ((it = symtable_find(stat->localSymbolTable, id)) != NULL) {
            if (!onlyFindDefinedSymbols || it->data.data.var_data.defined) {
                return stat->localSymbolTable;
            }
        }

        stat = stat->parentStatement;
    }

    return NULL;
}

MutableString make_var_name(const char *id, CFStatement *stat, bool isTF) {
    char *i = malloc(12);

    if (isTF) {
        sprintf(i, "1");
    } else {
        SymbolTable *symtab = find_sym_table(id, stat);
        sprintf(i, "%u", symtab->symbol_prefix);
    }

    MutableString str;
    mstr_make(&str, 5, isTF ? "TF" : "LF", "@$", i, "_", id);
    free(i);
    return str;
}

void print_var_name_id(const char *id, CFStatement *stat) {
    SymbolTable *symtab = find_sym_table(id, stat);
    out_nnl("%s@$%u_%s", "LF", symtab->symbol_prefix, id);
}

void print_var_name(ASTNode *idAstNode, CFStatement *stat) {
    STSymbol *st = idAstNode->data[0].symbolTableItemPtr;
    print_var_name_id(st->identifier, stat);
}

void print_var_name_or_const(ASTNode *node, CFStatement *stat) {
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
        print_var_name(node, stat);
    }
}

void generate_statement(CFStatement *stat);

void generate_assignment_for_varname(const char *varName, CFStatement *stat, ASTNode *value);

bool generate_expression_ast_result(ASTNode *exprAst, CFStatement *stat);

bool generate_logic_expression_tree(ASTNode *exprAst, CFStatement *stat, char *trueLabel, char *falseLabel);

void generate_print_log_expr(ASTNode *ast, CFStatement *stat) {
    unsigned counter = currentFunction.ifCounter;
    currentFunction.ifCounter++;

    char i[11];
    sprintf(i, "%i", counter);

    MutableString trueLabelStr, falseLabelStr;

    mstr_make(&trueLabelStr, 5, "$", stat->parentFunction->name, "_print", i, "_true");
    mstr_make(&falseLabelStr, 5, "$", stat->parentFunction->name, "_print", i, "_false");

    generate_logic_expression_tree(ast, stat, mstr_content(&trueLabelStr),
                                   mstr_content(&falseLabelStr));

    out("LABEL %s", mstr_content(&trueLabelStr));
    out("WRITE bool@true");
    out("JUMP $%s_print%i_end", stat->parentFunction->name, counter);
    out("LABEL %s", mstr_content(&falseLabelStr));
    out("WRITE bool@false");
    out("LABEL $%s_print%i_end", stat->parentFunction->name, counter);

    mstr_free(&trueLabelStr);
    mstr_free(&falseLabelStr);
}

void generate_print(ASTNode *argAstList, CFStatement *stat) {
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
                print_var_name(ast, stat);
                out_nl();
                break;
            default:
                ast_infer_node_type(ast);

                if (ast->inheritedDataType == CF_BOOL) {
                    generate_print_log_expr(ast, stat);

                } else {
                    generate_expression_ast_result(ast, stat);
                    out("POPS %s", PRINT_VAR);
                    out("WRITE %s", PRINT_VAR);
                }
                break;
        }
    }
}

void generate_internal_inputx(const char *expType, CFStatement *stat, const char *defaultValue) {
    unsigned counter = currentFunction.jumpingExprCounter++;

    // Read value into REG_1
    out("READ %s %s", REG_1, expType);
    // Write its type into REG_2
    out("TYPE %s %s", REG_2, REG_1);

    out("JUMPIFNEQ $%s_input%i_error %s string@%s", stat->parentFunction->name, counter,
        REG_2, expType);

    // Type matches -> push result and 0
    out("PUSHS int@0");
    out("PUSHS %s", REG_1);
    out("JUMP $%s_input%i_end", stat->parentFunction->name, counter);

    // Type doesn't match -> push default value of target type and 1
    out("LABEL $%s_input%i_error", stat->parentFunction->name, counter);
    out("PUSHS int@1");
    out("PUSHS %s@%s", expType, defaultValue);

    out("LABEL $%s_input%i_end", stat->parentFunction->name, counter);
}

void generate_internal_int2float(ASTNode *argAst, CFStatement *stat) {
    if (argAst->actionType != AST_LIST || argAst->dataCount != 1) {
        return;
    }

    generate_expression_ast_result(argAst->data[0].astPtr, stat);
    out("INT2FLOATS");
}

void generate_internal_float2int(ASTNode *argAst, CFStatement *stat) {
    if (argAst->actionType != AST_LIST || argAst->dataCount != 1) {
        return;
    }

    generate_expression_ast_result(argAst->data[0].astPtr, stat);
    out("FLOAT2INTS");
}

void generate_internal_len(ASTNode *argAst, CFStatement *stat) {
    if (argAst->actionType != AST_LIST || argAst->dataCount != 1) {
        return;
    }
    argAst = argAst->data[0].astPtr;

    if (argAst->actionType == AST_ID || argAst->actionType == AST_CONST_STRING) {
        out_nnl("STRLEN %s ", REG_1);
        print_var_name_or_const(argAst, stat);
        out_nl();
        out("PUSHS %s", REG_1);
    } else {
        generate_expression_ast_result(argAst, stat);
        out("POPS %s", REG_1);
        out_nnl("STRLEN %s %s", REG_1, REG_1);
        out("PUSHS %s", REG_1);
    }
}

void generate_internal_chr(ASTNode *argAst, CFStatement *stat) {
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

        generate_expression_ast_result(argAst, stat);
        out("POPS %s", REG_1);
        out("LT %s %s int@0", COND_RES_VAR, REG_1);
        out("JUMPIFEQ $%s_chr%i_fail %s bool@true", stat->parentFunction->name, counter, COND_RES_VAR);
        out("GT %s %s int@255", COND_RES_VAR, REG_1);
        out("JUMPIFEQ $%s_chr%i_fail %s bool@true", stat->parentFunction->name, counter, COND_RES_VAR);

        out("PUSHS int@0");
        out("PUSHS %s", REG_1);
        out("INT2CHARS");
        out("JUMP $%s_chr%i_end", stat->parentFunction->name, counter);

        out("LABEL $%s_chr%i_fail", stat->parentFunction->name, counter);
        out("PUSHS int@1");
        out("PUSHS string@");

        out("LABEL $%s_chr%i_end", stat->parentFunction->name, counter);
    }
}

void generate_internal_substr(ASTNode *argAst, CFStatement *stat) {
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
            ms = make_var_name(strArg->data[0].symbolTableItemPtr->identifier, stat, false);
            strArgOp = mstr_content(&ms);
        } else {
            generate_expression_ast_result(strArg, stat);
            out("POPS %s", REG_1);
            strArgOp = REG_1;
        }

        out("STRLEN %s %s", REG_2, strArgOp);
    }

    unsigned counter = currentFunction.jumpingExprCounter;
    currentFunction.jumpingExprCounter++;

    // move begin index to REG_3
    generate_assignment_for_varname(REG_3, stat, beginIndexArg);

    // REG_1: input string
    // REG_2: len(s)
    // REG_3: i

    // goto fail if i < 0
    out("LT %s %s int@0", COND_RES_VAR, REG_3);
    out("JUMPIFEQ $%s_substr%i_fail %s bool@true", stat->parentFunction->name, counter,
        COND_RES_VAR);
    // || i > len(s)
    out("GT %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_substr%i_fail %s bool@true", stat->parentFunction->name, counter,
        COND_RES_VAR);
    // || i == len(s)
    out("EQ %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_substr%i_fail %s bool@true", stat->parentFunction->name, counter,
        COND_RES_VAR);

    out("CREATEFRAME");
    out("DEFVAR TF@$tmp_i_%i", counter);
    out("DEFVAR TF@$tmp_res_%i", counter);

    out("MOVE TF@$tmp_i_%i %s", counter, REG_3);
    out("MOVE TF@$tmp_res_%i string@", counter);

    // REG_3: n
    generate_assignment_for_varname(REG_3, stat, lenArg);

    // goto fail if n < 0
    out("LT %s %s int@0", COND_RES_VAR, REG_3);
    out("JUMPIFEQ $%s_substr%i_fail %s bool@true", stat->parentFunction->name, counter,
        COND_RES_VAR);

    out("ADD %s %s TF@$tmp_i_%i", REG_3, REG_3, counter);

    // REG_1: input string
    // REG_2: len(s)
    // REG_3: i + n
    // LF@$tmp_i_%i: i
    // LF@$tmp_res_%i: (empty string)

    out("GT %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_substr%i_cont %s bool@false", stat->parentFunction->name, counter, COND_RES_VAR);
    // if n + i > len(s): REG_3 = lenS
    out("MOVE %s %s", REG_3, strLenArgOp);
    out("LABEL $%s_substr%i_cont", stat->parentFunction->name, counter);
    // while i < REG_3: append and increment i
    out("LT %s TF@$tmp_i_%i %s", COND_RES_VAR, counter, REG_3);
    out("JUMPIFEQ $%s_substr%i_forend %s bool@false", stat->parentFunction->name, counter, COND_RES_VAR);

    out("GETCHAR %s %s TF@$tmp_i_%i", REG_2, strArgOp, counter);
    out("CONCAT TF@$tmp_res_%i TF@$tmp_res_%i %s", counter, counter, REG_2);
    out("ADD TF@$tmp_i_%i TF@$tmp_i_%i int@1", counter, counter);
    out("JUMP $%s_substr%i_cont", stat->parentFunction->name, counter);

    out("LABEL $%s_substr%i_forend", stat->parentFunction->name, counter);
    out("PUSHS int@0");
    out("PUSHS TF@$tmp_res_%i", counter);
    out("JUMP $%s_substr%i_end", stat->parentFunction->name, counter);

    out("LABEL $%s_substr%i_fail", stat->parentFunction->name, counter);
    out("PUSHS int@1");
    out("PUSHS string@");
    out("LABEL $%s_substr%i_end", stat->parentFunction->name, counter);

    if (strArg->actionType == AST_ID) {
        mstr_free(&ms);
    } else if (strArg->actionType == AST_CONST_STRING) {
        free(strArgOp);
        free(strLenArgOp);
    }
}

void generate_internal_ord(ASTNode *argAst, CFStatement *stat) {
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
            ms = make_var_name(strArg->data[0].symbolTableItemPtr->identifier, stat, false);
            strArgOp = mstr_content(&ms);
        } else {
            generate_expression_ast_result(strArg, stat);
            out("POPS %s", REG_1);
            strArgOp = REG_1;
        }

        out("STRLEN %s %s", REG_2, strArgOp);
    }

    unsigned counter = currentFunction.jumpingExprCounter;
    currentFunction.jumpingExprCounter++;

    // move begin index to REG_3
    generate_assignment_for_varname(REG_3, stat, beginIndexArg);

    // REG_1: input string
    // REG_2: len(s)
    // REG_3: i

    // goto fail if i < 0
    out("LT %s %s int@0", COND_RES_VAR, REG_3);
    out("JUMPIFEQ $%s_ord%i_fail %s bool@true", stat->parentFunction->name, counter,
        COND_RES_VAR);
    // || i > len(s)
    out("GT %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_ord%i_fail %s bool@true", stat->parentFunction->name, counter,
        COND_RES_VAR);
    // || i == len(s)
    out("EQ %s %s %s", COND_RES_VAR, REG_3, strLenArgOp);
    out("JUMPIFEQ $%s_ord%i_fail %s bool@true", stat->parentFunction->name, counter,
        COND_RES_VAR);

    out("STRI2INT %s %s %s", REG_2, strArgOp, REG_3);
    out("PUSHS int@0");
    out("PUSHS %s", REG_2);
    out("JUMP $%s_ord%i_end", stat->parentFunction->name, counter);

    out("LABEL $%s_ord%i_fail", stat->parentFunction->name, counter);
    out("PUSHS int@1");
    out("PUSHS string@");
    out("LABEL $%s_ord%i_end", stat->parentFunction->name, counter);

    if (strArg->actionType == AST_ID) {
        mstr_free(&ms);
    } else if (strArg->actionType == AST_CONST_STRING) {
        free(strArgOp);
        free(strLenArgOp);
    }
}

bool generate_internal_func_call(ASTNode *funcCallAst, CFStatement *stat) {
    STSymbol *s = funcCallAst->left->data[0].symbolTableItemPtr;
    ASTNode *args = funcCallAst->right;

    if (s == symbs.print) {
        generate_print(args, stat);
        return true;
    } else if (s == symbs.int2float) {
        generate_internal_int2float(args, stat);
        return true;
    } else if (s == symbs.float2int) {
        generate_internal_float2int(args, stat);
        return true;
    } else if (s == symbs.len) {
        generate_internal_len(args, stat);
        return true;
    } else if (s == symbs.ord) {
        generate_internal_ord(args, stat);
        return true;
    } else if (s == symbs.chr) {
        generate_internal_chr(args, stat);
        return true;
    } else if (s == symbs.substr) {
        generate_internal_substr(args, stat);
        return true;
    } else if (s == symbs.inputi) {
        generate_internal_inputx("int", stat, "0");
        return true;
    } else if (s == symbs.inputb) {
        generate_internal_inputx("bool", stat, "false");
        return true;
    } else if (s == symbs.inputs) {
        generate_internal_inputx("string", stat, "");
        return true;
    } else if (s == symbs.inputf) {
        generate_internal_inputx("float", stat, "0x0p+0");
        return true;
    }

    return false;
}

void generate_func_call(ASTNode *funcCallAst, CFStatement *stat) {
    // Arguments are passed in a temporary frame
    // The frame will then be pushed as LF in the function itself, if it makes sense
    if (is_ast_empty(funcCallAst) || funcCallAst->left == NULL) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Function call has no target.\n");
        return;
    }

    STSymbol *targetFuncSymb = funcCallAst->left->data[0].symbolTableItemPtr;
    CFFunction *targetFunc = cf_get_function(targetFuncSymb->identifier, false);
    dbg("Generating func call to '%s'", targetFuncSymb->identifier);

    ASTNode *argAstList = funcCallAst->right;

    if (generate_internal_func_call(funcCallAst, stat)) {
        return;
    }

    out("CREATEFRAME");

    if (targetFunc->argumentsCount > 0) {
        if (argAstList == NULL || argAstList->dataCount != targetFunc->argumentsCount) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                           "Function call has unexpected amount of arguments.\n");
            return;
        }

        STParam *argN = targetFuncSymb->data.func_data.params;
        ASTNodeData *argContainer = argAstList->data;

        while (argN != NULL) {
            MutableString varName = make_var_name(argN->id, targetFunc->rootStatement, true);

            ASTNode *argData = argContainer->astPtr;
            out("DEFVAR %s", mstr_content(&varName));
            generate_assignment_for_varname(mstr_content(&varName), stat, argData);

            mstr_free(&varName);

            argN = argN->next;
            argContainer++;
        }
    }

    out("CALL %s", targetFuncSymb->identifier);
    dbg("Function return values are on stack");

    // The return values are on stack
}

void generate_expression_ast(ASTNode *exprAst, CFStatement *stat) {
    switch (exprAst->actionType) {
        case AST_ID:
            out_nnl("PUSHS ");
            print_var_name(exprAst, stat);
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

// Target: 0 = push to stack, 1 = REG_1, 2 = REG_2
void generate_string_concat(ASTNode *addAst, CFStatement *stat, int target) {
    ASTNode *left = addAst->left;
    ASTNode *right = addAst->right;

    if (is_direct_ast(left) && is_direct_ast(right)) {
        out_nnl("CONCAT %s ", target == 2 ? REG_2 : REG_1);
        print_var_name_or_const(left, stat);
        out_nnl(" ");
        print_var_name_or_const(right, stat);
        out_nl();
    } else if (is_direct_ast(left)) {
        generate_string_concat(right, stat, 1);

        out_nnl("CONCAT %s ", target == 2 ? REG_2 : REG_1);
        print_var_name_or_const(left, stat);
        out("%s", target == 2 ? REG_2 : REG_1);
    } else if (is_direct_ast(right)) {
        generate_string_concat(left, stat, 1);

        out_nnl("CONCAT %s %s ", target == 2 ? REG_2 : REG_1, REG_1);
        print_var_name_or_const(right, stat);
        out_nl();
    } else {
        generate_string_concat(left, stat, 2);
        generate_string_concat(right, stat, 1);

        out("CONCAT %s %s %s", target == 2 ? REG_2 : REG_1, REG_2, REG_1);
    }

    if (target == 0) {
        out("PUSHS %s", REG_1);
    }
}

bool generate_expression_ast_result(ASTNode *exprAst, CFStatement *stat) {
    if (exprAst == NULL) {
        dbg("Null expression");
        return false;
    }

    if (exprAst->actionType == AST_FUNC_CALL) {
        generate_func_call(exprAst, stat);
        return true;
    }

    if (exprAst->actionType == AST_ASSIGN || exprAst->actionType == AST_DEFINE) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                       "Unexpected assignment/definition.\n");
        return false;
    }

    if (exprAst->actionType == AST_AR_NEGATE) {
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
            generate_string_concat(exprAst, stat, 0);
        } else if (exprAst->actionType == AST_ID) {
            out_nnl("PUSHS ");
            print_var_name(exprAst, stat);
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
        generate_expression_ast_result(exprAst->left, stat);
    if (exprAst->right != NULL)
        generate_expression_ast_result(exprAst->right, stat);

    generate_expression_ast(exprAst, stat);
    return true;
}

char *make_next_logic_label() {
    static int cntr = 0;
    char *a = malloc(11);
    sprintf(a, "$$log_%i", cntr++);
    return a;
}

bool generate_logic_expression_assignment(ASTNode *exprAst, CFStatement *stat, const char *targetVarName);

bool generate_simple_logic_expression(ASTNode *exprAst, CFStatement *stat, char *trueLabel, char *falseLabel) {
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
        print_var_name(exprAst, stat);
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

        generate_func_call(exprAst, stat);
        // One bool return value on stack
        out("PUSHS bool@true");
        out("JUMPIFEQS %s", trueLabel);
        out("JUMP %s", falseLabel);
        return true;
    }

    if (is_direct_ast(left) && is_direct_ast(right)) {
        if (t == AST_LOG_EQ || t == AST_LOG_NEQ) {
            out_nnl("%s %s ", t == AST_LOG_EQ ? "JUMPIFEQ" : "JUMPIFNEQ", trueLabel);
            print_var_name_or_const(left, stat);
            out_nnl(" ");
            print_var_name_or_const(right, stat);
            out_nl();
        } else if (t == AST_LOG_LT || t == AST_LOG_GT) {
            out_nnl("%s %s ", t == AST_LOG_LT ? "LT" : "GT", COND_RES_VAR);
            print_var_name_or_const(left, stat);
            out_nnl(" ");
            print_var_name_or_const(right, stat);
            out_nl();
            out("JUMPIFEQ %s bool@true %s", trueLabel, COND_RES_VAR);

        } else if (t == AST_LOG_LTE || t == AST_LOG_GTE) {
            out_nnl("JUMPIFEQ %s ", trueLabel);
            print_var_name_or_const(left, stat);
            out_nnl(" ");
            print_var_name_or_const(right, stat);
            out_nl();

            out_nnl("%s %s ", t == AST_LOG_LTE ? "LT" : "GT", COND_RES_VAR);
            print_var_name_or_const(left, stat);
            out_nnl(" ");
            print_var_name_or_const(right, stat);
            out_nl();

            out("JUMPIFEQ %s bool@true %s", trueLabel, COND_RES_VAR);
        } else {
            // error
            return false;
        }

        out("JUMP %s", falseLabel);
        return true;
    } else {
        if (left->inheritedDataType == CF_BOOL && right->inheritedDataType == CF_BOOL) {
            // This could be optimised quite easily
            generate_logic_expression_assignment(left, stat, NULL);
            generate_logic_expression_assignment(right, stat, NULL);
        } else if (left->inheritedDataType == CF_BOOL || right->inheritedDataType == CF_BOOL) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                           "Unexpected value in logical expression.\n");
            return false;
        } else {
            generate_expression_ast_result(left, stat);
            generate_expression_ast_result(right, stat);
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
        } else {
            // todo: error
            return false;
        }

        out("JUMP %s", falseLabel);
        return true;
    }
}

bool generate_logic_expression_tree(ASTNode *exprAst, CFStatement *stat, char *trueLabel, char *falseLabel) {
    ASTNode *left = exprAst->left;
    ASTNode *right = exprAst->right;

    bool result = true;
    if (exprAst->actionType == AST_LOG_AND) {
        char *next = make_next_logic_label();

        result &= generate_logic_expression_tree(left, stat, next, falseLabel);
        out("LABEL %s", next);
        result &= generate_logic_expression_tree(right, stat, trueLabel, falseLabel);

        free(next);
    } else if (exprAst->actionType == AST_LOG_OR) {
        char *next = make_next_logic_label();

        result &= generate_logic_expression_tree(left, stat, trueLabel, next);
        out("LABEL %s", next);
        result &= generate_logic_expression_tree(right, stat, trueLabel, falseLabel);

        free(next);
    } else if (exprAst->actionType == AST_LOG_NOT) {
        return generate_logic_expression_tree(left, stat, falseLabel, trueLabel);
    } else {
        return generate_simple_logic_expression(exprAst, stat, trueLabel, falseLabel);
    }

    return result;
}

bool generate_logic_expression_assignment(ASTNode *exprAst, CFStatement *stat, const char *targetVarName) {
    unsigned counter = currentFunction.jumpingExprCounter;
    currentFunction.jumpingExprCounter++;

    char i[11];
    sprintf(i, "%i", counter);

    MutableString trueLabelStr, falseLabelStr;
    mstr_make(&trueLabelStr, 5, "$", stat->parentFunction->name, "_", i, "_true");
    mstr_make(&falseLabelStr, 5, "$", stat->parentFunction->name, "_", i, "_false");

    generate_logic_expression_tree(exprAst, stat, mstr_content(&trueLabelStr), mstr_content(&falseLabelStr));
    out("LABEL %s", mstr_content(&trueLabelStr));
    if (targetVarName == NULL) {
        out("PUSHS bool@true");
    } else {
        out("MOVE %s bool@true", targetVarName);
    }

    out("JUMP $%s_%i_end", stat->parentFunction->name, counter);

    out("LABEL %s", mstr_content(&falseLabelStr));
    if (targetVarName == NULL) {
        out("PUSHS bool@false");
    } else {
        out("MOVE %s bool@false", targetVarName);
    }

    out("LABEL $%s_%i_end", stat->parentFunction->name, counter);

    mstr_free(&trueLabelStr);
    mstr_free(&falseLabelStr);

    return true;
}

void generate_assignment_for_varname(const char *varName, CFStatement *stat, ASTNode *value) {
    if (varName == NULL) {
        if (value->actionType >= AST_LOGIC && value->actionType < AST_CONTROL) {
            generate_logic_expression_assignment(value, stat, REG_1);
        } else {
            // The right side is not a CONST or ID, evaluate expression on stack and pop it.
            generate_expression_ast_result(value, stat);
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
            print_var_name(value, stat);
            out_nl();
            break;
        default:
            if (value->actionType >= AST_LOGIC && value->actionType < AST_CONTROL) {
                generate_logic_expression_assignment(value, stat, varName);
            } else {
                // The right side is not a CONST or ID, evaluate expression on stack and pop it.
                generate_expression_ast_result(value, stat);
                out("POPS %s", varName);
            }
            break;
    }
}

void generate_assignment(ASTNode *asgAst, CFStatement *stat) {
    if (asgAst->left->actionType == AST_LIST) {
        if (asgAst->right->actionType == AST_LIST && asgAst->right->dataCount == 1
            && asgAst->right->data[0].astPtr->actionType == AST_FUNC_CALL) {

            STSymbol *funcSymb = asgAst->right->data[0].astPtr->left->data[0].symbolTableItemPtr;

            if (asgAst->left->dataCount != funcSymb->data.func_data.ret_types_count) {
                stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                               "Assignment left-hand side variables don't match the right-hand side function's return values.\n");
                return;
            }

            generate_func_call(asgAst->right->data[0].astPtr, stat);

            for (unsigned i = 0; i < asgAst->left->dataCount; i++) {
                if (asgAst->left->data[i].astPtr->inheritedDataType == CF_BLACK_HOLE ||
                    asgAst->left->data[i].astPtr->data[0].symbolTableItemPtr->reference_counter == 0) {
                    out("POPS %s", REG_1);
                    continue;
                }

                MutableString varName = make_var_name(
                        asgAst->left->data[i].astPtr->data[0].symbolTableItemPtr->identifier, stat, false);
                out("POPS %s", mstr_content(&varName));
                mstr_free(&varName);
            }

            return;
        }

        if (asgAst->right->actionType != AST_LIST) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                           "Expected AST_LIST on the right side, got %i instead.\n", asgAst->right->actionType);
        }

        if (asgAst->left->dataCount != asgAst->right->dataCount) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                           "Assignment left-hand side variable count doesn't match the right-hand side variable count.\n");
            return;
        }

        for (unsigned i = 0; i < asgAst->left->dataCount; i++) {
            ASTNode *tmpAssignNode = ast_node(AST_ASSIGN);
            ASTNode *tmpAssignLeft = asgAst->left->data[i].astPtr;
            tmpAssignNode->left = tmpAssignLeft;
            tmpAssignNode->right = asgAst->right->data[i].astPtr;
            generate_assignment(tmpAssignNode, stat);
            free(tmpAssignNode);
        }

        return;
    }

    if (asgAst->left->inheritedDataType == CF_BLACK_HOLE) {
        generate_assignment_for_varname(NULL, stat, asgAst->right);
    } else if (asgAst->left->data[0].symbolTableItemPtr->reference_counter == 0) {
        dbg("Omitting assignment into unused variable");
    } else {
        MutableString varName = make_var_name(asgAst->left->data[0].symbolTableItemPtr->identifier, stat, false);
        onlyFindDefinedSymbols = true;
        generate_assignment_for_varname(mstr_content(&varName), stat, asgAst->right);
        onlyFindDefinedSymbols = false;
        asgAst->left->data[0].symbolTableItemPtr->data.var_data.defined = true;
        mstr_free(&varName);
    }
}

void generate_definition(ASTNode *defAst, CFStatement *stat) {
    generate_assignment(defAst, stat);
}

void generate_return_statement(CFStatement *stat) {
    ASTNode *retAstList = stat->data.bodyAst;

    if (currentFunction.isMain && !currentFunction.generateMainAsFunction) {
        if (retAstList != NULL && retAstList->dataCount != 0) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Expected an empty return statement for function 'main'.\n");
            return;
        }

        out("EXIT int@0");
        stat->parentFunction->terminated = true;
        return;
    }

    bool hasNamedReturnValues = currentFunction.function->returnValues != NULL
                                && currentFunction.function->returnValues->variable.name != NULL;

    if (stat->parentFunction->returnValuesCount == 0) {
        if (retAstList != NULL && retAstList->dataCount > 0) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Expected an empty return statement for function '%s'.\n",
                           stat->parentFunction->name);
            return;
        }
    } else {
        if (!hasNamedReturnValues && (retAstList == NULL || retAstList->actionType != AST_LIST)) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                           "Unexpected AST in RETURN statement.\n");
            return;
        }

        if (hasNamedReturnValues && retAstList != NULL && retAstList->dataCount != 0
            && retAstList->dataCount != stat->parentFunction->returnValuesCount) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Return statement of the function '%s' with named return values should either explicitly specify all return values or contain none.\n",
                           stat->parentFunction->name);
            return;
        }

        if (!hasNamedReturnValues && retAstList->dataCount != stat->parentFunction->returnValuesCount) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Return statement data count doesn't match function's '%s' return values count.\n",
                           stat->parentFunction->name);
            return;
        }
    }

    CFVarListNode *argNode = currentFunction.function->returnValues;
    if (retAstList != NULL && (!hasNamedReturnValues || retAstList->dataCount > 0)) {
        for (unsigned i = 0; i < retAstList->dataCount; i++) {
            ASTNode *ast = retAstList->data[retAstList->dataCount - i - 1].astPtr;

            if (!ast_infer_node_type(ast)) {
                stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,
                               "Types of values in return statement of function '%s' couldn't be inferred.\n",
                               stat->parentFunction->name);
                return;
            }

            if (ast->inheritedDataType != argNode->variable.dataType) {
                stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                               "Function '%s': Invalid return value type (return value on index %u).\n",
                               currentFunction.function->name, retAstList->dataCount - i - 1);
                return;
            }

            if (ast->actionType >= AST_LOGIC && ast->actionType < AST_CONTROL) {
                generate_logic_expression_assignment(ast, stat, REG_1);
            } else {
                generate_expression_ast_result(ast, stat);
            }

            argNode = argNode->next;
        }
    } else {
        for (unsigned i = 0; i < currentFunction.function->returnValuesCount; i++) {
            out_nnl("PUSHS ");
            print_var_name_id(argNode->variable.name, stat);
            out_nl();

            argNode = argNode->next;
        }
    }

    // Delete the local frame
    out("POPFRAME");
    out("RETURN");
    stat->parentFunction->terminated = true;
}

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

    char i[11];
    sprintf(i, "%i", counter);

    MutableString trueLabelStr, falseLabelStr;

    mstr_make(&trueLabelStr, 5, "$", stat->parentFunction->name, "_if", i, "_then");
    mstr_make(&falseLabelStr, 5, "$", stat->parentFunction->name, "_if", i, hasElse ? "_else" : "_end");

    generate_logic_expression_tree(stat->data.ifData->conditionalAst, stat, mstr_content(&trueLabelStr),
                                   mstr_content(&falseLabelStr));

    out("LABEL %s", mstr_content(&trueLabelStr));
    generate_statement(stat->data.ifData->thenStatement);

    if (hasElse) {
        out("JUMP $%s_if%i_end", stat->parentFunction->name, counter);
        out("LABEL %s", mstr_content(&falseLabelStr));
        generate_statement(stat->data.ifData->elseStatement);
    }

    out("LABEL $%s_if%i_end", stat->parentFunction->name, counter);

    mstr_free(&trueLabelStr);
    mstr_free(&falseLabelStr);
    dbg("Finished if #%i", counter);
}

void generate_for_statement(CFStatement *stat) {
    dbg("Generating for statement (if #%i)", currentFunction.ifCounter);

    unsigned counter = currentFunction.ifCounter;
    currentFunction.ifCounter++;

    if (stat->data.forData->definitionAst != NULL) {
        ast_infer_node_type(stat->data.forData->definitionAst);
        generate_definition(stat->data.forData->definitionAst, stat);
    }

    out("LABEL $%s_for%i_begin", stat->parentFunction->name, counter);

    if (stat->data.forData->conditionalAst != NULL) {
        ast_infer_node_type(stat->data.forData->conditionalAst);
        if (stat->data.forData->conditionalAst->inheritedDataType != CF_BOOL) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                           "Unexpected non-logical expression in for definition.\n");
            return;
        }

        char i[11];
        sprintf(i, "%i", counter);

        MutableString trueLabelStr, falseLabelStr;

        mstr_make(&trueLabelStr, 5, "$", stat->parentFunction->name, "_for", i, "_then");
        mstr_make(&falseLabelStr, 5, "$", stat->parentFunction->name, "_for", i, "_end");

        generate_logic_expression_tree(stat->data.forData->conditionalAst, stat, mstr_content(&trueLabelStr),
                                       mstr_content(&falseLabelStr));

        out("LABEL %s", mstr_content(&trueLabelStr));
        mstr_free(&trueLabelStr);
        mstr_free(&falseLabelStr);
    }

    generate_statement(stat->data.forData->bodyStatement);

    if (stat->data.forData->afterthoughtAst != NULL) {
        ast_infer_node_type(stat->data.forData->afterthoughtAst);
        generate_assignment(stat->data.forData->afterthoughtAst, stat);
    }

    out("JUMP $%s_for%i_begin", stat->parentFunction->name, counter);
    out("LABEL $%s_for%i_end", stat->parentFunction->name, counter);

    dbg("Finished for (if #%i)", counter);
}

void generate_basic_statement(CFStatement *stat) {
    if (stat->data.bodyAst == NULL) return;
    ast_infer_node_type(stat->data.bodyAst);

    switch (stat->data.bodyAst->actionType) {
        case AST_FUNC_CALL:
            if (stat->data.bodyAst->left->data[0].symbolTableItemPtr->data.func_data.ret_types_count > 0) {
                stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                               "Unexpected call outside an assigment or an expression to a function that returns values.");
                return;
            }

            generate_func_call(stat->data.bodyAst, stat);
            break;
        case AST_DEFINE:
            generate_definition(stat->data.bodyAst, stat);
            break;
        case AST_ASSIGN:
            generate_assignment(stat->data.bodyAst, stat);
            break;
        default: // sanity check, shouldn't get here
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,
                           "Invalid top-level statement, expected definition, assignment or function call.\n");
            return;
    }
}

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
                generate_return_statement(stat);
                break;
        }
    }

    if (stat->followingStatement != NULL) {
        generate_statement(stat->followingStatement);
    }
}

void generate_definitions(CFStatement *stat) {
    if (stat == NULL) return;

    if (stat->localSymbolTable->symbol_prefix == 0) {
        stat->localSymbolTable->symbol_prefix = currentFunction.scopeCounter++;

        for (unsigned ai = 0; ai < stat->localSymbolTable->arr_size; ai++) {
            STItem *it = stat->localSymbolTable->arr[ai];
            while (it != NULL) {
                STSymbol *symb = &it->data;

                if (symb->reference_counter > 0 && symb->type == ST_SYMBOL_VAR) {
                    if (!symb->data.var_data.is_argument_variable) {
                        MutableString varName = make_var_name(symb->identifier, stat, false);
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

void generate_function(CFFunction *fun) {
    dbg("Function '%s'", fun->name);

    struct cfgraph_program_structure *prog = get_program();
    STItem *funcSymbol = symtable_find(prog->globalSymtable, fun->name);
    if (funcSymbol->data.reference_counter == 0) {
        dbg("Function not used");
        stderr_message("codegen", WARNING, COMPILER_RESULT_SUCCESS, "Function '%s' is not used anywhere.\n",
                       fun->name);
        //return;
    }

    if (is_statement_empty(fun->rootStatement)) {
        stderr_message("codegen", WARNING, COMPILER_RESULT_SUCCESS, "Function '%s' is empty.\n", fun->name);

        if (fun->returnValuesCount > 0 && fun->returnValues->variable.name == NULL) {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Empty function with parameters is missing a return statement.");
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

    out("LABEL %s", fun->name);

    // In case of typical functions, a TF with local variables would be created before the call.
    // Main must make its own frame.
    if (currentFunction.isMain && !currentFunction.generateMainAsFunction) {
        out("CREATEFRAME");
    }

    // OPTSUG: Pushing the frame could be omitted if the function only uses its arguments
    // (all uses of LF@x should be then generated as TF@x instead)
    out("PUSHFRAME");

    generate_definitions(fun->rootStatement);
    generate_statement(fun->rootStatement);

    // Return from the function will be generated from the first RETURN statement
    if (!fun->terminated) {
        if (fun->returnValuesCount == 0 || fun->returnValues->variable.name != NULL) {
            ASTNode *astBackup = fun->rootStatement->data.bodyAst;
            fun->rootStatement->data.bodyAst = NULL;
            generate_return_statement(fun->rootStatement);
            fun->rootStatement->data.bodyAst = astBackup;
        } else {
            stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "Function '%s' is missing a return statement.\n", fun->name);
            return;
        }
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

    ast_set_strict_inference_state(true);

    CFProgram *prog = get_program();
    CFFuncListNode *n = prog->functionList;

    STItem *mainSym = symtable_find(prog->globalSymtable, "main");
    if (prog->mainFunc == NULL || mainSym == NULL) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE,
                       "Undefined main function.\n");
        return;
    }
    bool generateMainAsFunc = mainSym->data.reference_counter > 1;

    out(".IFJcode20");

    out("DEFVAR %s", COND_RES_VAR);
    out("DEFVAR %s", COND_LHS_VAR);
    out("DEFVAR %s", COND_RHS_VAR);
    out("DEFVAR %s", REG_1);
    out("DEFVAR %s", REG_2);

    find_internal_symbols(prog->globalSymtable);
    if (symbs.printUsed) {
        out("DEFVAR %s", PRINT_VAR);
    }
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
        n = n->next;
    }
}
