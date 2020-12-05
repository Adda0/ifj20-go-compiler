/** @file parser.c
 *
 * IFJ20 compiler
 *
 * @brief Implements the recursive descent parser.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#include <string.h>
#include "parser.h"
#include "compiler.h"
#include "scanner.h"
#include "mutable_string.h"
#include "stderr_message.h"
#include "precedence_parser.h"
#include "stacks.h"
#include "symtable.h"
#include "control_flow.h"

Token token, prev_token;
ScannerResult scanner_result;
SymtableStack symtable_stack;
SymbolTable *function_table;

int else_();

ScannerResult result_if_eol_ok(ScannerResult peeked_result) {
    if (peeked_result != SCANNER_RESULT_EXCESS_EOL && peeked_result != SCANNER_RESULT_MISSING_EOL) {
        return peeked_result;
    } else {
        return SCANNER_RESULT_SUCCESS;
    }
}

ScannerResult calculate_new_scanner_result(ScannerResult peeked_result, EolRule eol, bool eol_read) {
    if (eol == EOL_FORBIDDEN) {
        if (eol_read) {
            return SCANNER_RESULT_EXCESS_EOL;
        } else {
            return result_if_eol_ok(peeked_result);
        }
    } else if (eol == EOL_REQUIRED) {
        if (eol_read) {
            return result_if_eol_ok(peeked_result);
        } else {
            return SCANNER_RESULT_MISSING_EOL;
        }
    } else {
        return result_if_eol_ok(peeked_result);
    }
}

int get_token(Token *token, EolRule eol, bool peek_only) {
    static ScannerResult peeked_result;
    static Token peeked_token;
    static bool peeked = false;
    if (peeked) {
        if (!peek_only) {
            peeked = false;
        }
        *token = peeked_token;
        // We have to return based on the new eol rule
        return calculate_new_scanner_result(peeked_result, eol, peeked_token.context.eol_read);
    } else {
        if (peek_only) {
            peeked_result = scanner_get_token(&peeked_token, eol);
            *token = peeked_token;
            peeked = true;
            return peeked_result;
        } else {
            return scanner_get_token(token, eol);
        }
    }
}

char *convert_token_to_text() {
    switch (token.type) {
        case TOKEN_DEFAULT:
            return "undefined";
        case TOKEN_ID:
            return "identifier";
        case TOKEN_INT:
            return "int value";
        case TOKEN_FLOAT:
            return "float value";
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_BOOL:
                    return "keyword bool";
                case KEYWORD_ELSE:
                    return "keyword else";
                case KEYWORD_FLOAT64:
                    return "keyword float64";
                case KEYWORD_FOR:
                    return "keyword for";
                case KEYWORD_FUNC:
                    return "keyword func";
                case KEYWORD_IF:
                    return "keyword if";
                case KEYWORD_INT:
                    return "keyword int";
                case KEYWORD_PACKAGE:
                    return "keyword package";
                case KEYWORD_RETURN:
                    return "keyword return";
                case KEYWORD_STRING:
                    return "keyword string";
                default:
                    return "";
            }
        case TOKEN_PLUS:
            return "+";
        case TOKEN_MINUS:
            return "-";
        case TOKEN_MULTIPLY:
            return "*";
        case TOKEN_DIVIDE:
            return "/";
        case TOKEN_PLUS_ASSIGN:
            return "+=";
        case TOKEN_MINUS_ASSIGN:
            return "-=";
        case TOKEN_MULTIPLY_ASSIGN:
            return "*=";
        case TOKEN_DIVIDE_ASSIGN:
            return "/=";
        case TOKEN_DEFINE:
            return ":=";
        case TOKEN_ASSIGN:
            return "=";
        case TOKEN_EQUAL_TO:
            return "==";
        case TOKEN_BOOL:
            return "bool value";
        case TOKEN_NOT:
            return "!";
        case TOKEN_NOT_EQUAL_TO:
            return "!=";
        case TOKEN_AND:
            return "&&";
        case TOKEN_OR:
            return "||";
        case TOKEN_LEFT_BRACKET:
            return "(";
        case TOKEN_RIGHT_BRACKET:
            return ")";
        case TOKEN_CURLY_LEFT_BRACKET:
            return "{";
        case TOKEN_CURLY_RIGHT_BRACKET:
            return "}";
        case TOKEN_LESS_THAN:
            return "<";
        case TOKEN_GREATER_THAN:
            return ">";
        case TOKEN_LESS_OR_EQUAL:
            return "<=";
        case TOKEN_GREATER_OR_EQUAL:
            return ">=";
        case TOKEN_STRING:
            return "string value";
        case TOKEN_COMMA:
            return ",";
        case TOKEN_SEMICOLON:
            return ";";
        default:
            return "";
    }
}

void clear_token() {
    if (token.type == TOKEN_ID || token.type == TOKEN_STRING) {
        mstr_free(&token.data.str_val);
    }
}

int type(STDataType *data_type) {
    if (token.type != TOKEN_KEYWORD) {
        token_error("expected float64, int, string or bool keyword, got %s\n");
        syntax_error();
    }
    switch (token.data.keyword_type) {
        case KEYWORD_FLOAT64: // rule <type> -> float64
            *data_type = CF_FLOAT;
            break;
        case KEYWORD_INT:     // rule <type> -> int
            *data_type = CF_INT;
            break;
        case KEYWORD_STRING:  // rule <type> -> string
            *data_type = CF_STRING;
            break;
        case KEYWORD_BOOL:    // rule <type> -> bool
            *data_type = CF_BOOL;
            break;
        default:
            token_error("expected float64, int, string or bool keyword, got %s\n");
            syntax_error();
    }
    check_new_token(EOL_FORBIDDEN);
    syntax_ok();
}

int params_n(STItem *current_function, bool ret_type, bool already_found, STParam *current_param) {
    MutableString id;
    switch (token.type) {
        case TOKEN_RIGHT_BRACKET:
            // rule <params_n> -> eps
            syntax_ok();
        case TOKEN_COMMA:
            // rule <params_n> -> , id <type> <params_n>
            // we've read comma, newline is allowed according to FUNEXP
            check_new_token(EOL_OPTIONAL);
            if (token.type != TOKEN_ID) {
                token_error("expected identifier, got %s\n");
                syntax_error();
            }
            id = token.data.str_val;
            check_new_token(EOL_FORBIDDEN);
            STDataType data_type;
            check_nonterminal(type(&data_type));
            STParam *next_param = NULL;
            if (semantic_enabled) {
                if (ret_type) {
                    check_cf(cf_add_return_value(mstr_content(&id), data_type));
                    if (!symtable_add_ret_type(current_function, mstr_content(&id), data_type)) {
                        return COMPILER_RESULT_ERROR_INTERNAL;
                    }
                } else {
                    check_cf(cf_add_argument(mstr_content(&id), data_type));
                    if (already_found) {
                        if (current_param == NULL) {
                            stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                                           "Line %u, col %u: unexpected parameter to function\n",
                                           prev_token.context.line_num, prev_token.context.char_num);
                            return COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE;
                        }
                        if (current_param->type != CF_UNKNOWN && current_param->type != data_type) {
                            stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                                           "Line %u, col %u: wrong param type to function\n",
                                           prev_token.context.line_num, prev_token.context.char_num);
                            return COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE;
                        }
                        // Update the information
                        char *new_buffer = malloc(sizeof(char) * (strlen(mstr_content(&id)) + 1));
                        if (new_buffer == NULL) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                        strcpy(new_buffer, mstr_content(&id));
                        current_param->id = new_buffer;
                        current_param->type = data_type;
                        next_param = current_param->next;

                    } else {
                        if (!symtable_add_param(current_function, mstr_content(&id), data_type)) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                    }
                }
                STItem *var = symtable_add(symtable_stack_top(&symtable_stack)->table, mstr_content(&id),
                                           ST_SYMBOL_VAR);
                if (var == NULL) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                var->data.data.var_data.type = data_type;
                var->data.data.var_data.defined = true;

                if (ret_type) {
                    var->data.data.var_data.is_return_val_variable = true;
                    var->data.reference_counter = 1;
                } else {
                    var->data.data.var_data.is_argument_variable = true;
                }
            }
            mstr_free(&id);
            return params_n(current_function, ret_type, already_found, next_param);
        default:
            token_error("expected ) or , when parsing parameters, got %s\n");
            syntax_error();
    }
}

int params(STItem *current_function, bool ret_type, bool already_found) {
    MutableString id;
    switch (token.type) {
        case TOKEN_RIGHT_BRACKET:
            // rule <params> -> eps
            syntax_ok();
        case TOKEN_ID:
            // rule <params> -> id <type> <params_n>
            id = token.data.str_val;
            check_new_token(EOL_FORBIDDEN);
            STDataType data_type;
            check_nonterminal(type(&data_type));
            STParam *next_param = NULL;
            if (semantic_enabled) {
                if (ret_type) {
                    check_cf(cf_add_return_value(mstr_content(&id), data_type));
                    if (!symtable_add_ret_type(current_function, mstr_content(&id), data_type)) {
                        return COMPILER_RESULT_ERROR_INTERNAL;
                    }
                } else {
                    check_cf(cf_add_argument(mstr_content(&id), data_type));
                    if (already_found) {
                        // Check the type of the first param if we predicted arguments in an expression
                        STParam *first_param = current_function->data.data.func_data.params;
                        if (first_param == NULL) {
                            stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                                           "Line %u, col %u: unexpected parameter to function\n",
                                           prev_token.context.line_num, prev_token.context.char_num);
                            return COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE;
                        }
                        if (first_param->type != CF_UNKNOWN && first_param->type != data_type) {
                            stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                                           "Line %u, col %u: wrong param type to function\n",
                                           prev_token.context.line_num, prev_token.context.char_num);
                            return COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE;
                        }
                        // Update the information
                        char *new_buffer = malloc(sizeof(char) * (strlen(mstr_content(&id)) + 1));
                        if (new_buffer == NULL) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                        strcpy(new_buffer, mstr_content(&id));
                        first_param->id = new_buffer;
                        first_param->type = data_type;
                        next_param = first_param->next;
                    } else {
                        if (!symtable_add_param(current_function, mstr_content(&id), data_type)) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                    }
                }
                STItem *var = symtable_add(symtable_stack_top(&symtable_stack)->table, mstr_content(&id),
                                           ST_SYMBOL_VAR);
                if (var == NULL) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                var->data.data.var_data.type = data_type;
                var->data.data.var_data.defined = true;

                if (ret_type) {
                    var->data.data.var_data.is_return_val_variable = true;
                    var->data.reference_counter = 1;
                } else {
                    var->data.data.var_data.is_argument_variable = true;
                }
            }
            mstr_free(&id);
            return params_n(current_function, ret_type, already_found, next_param);
        default:
            token_error("expected ) or identifier when parsing parameters, got %s\n");
            syntax_error();
    }
}

int else_n() {
    SymbolTable *new_body_table;
    switch (token.type) {
        case TOKEN_CURLY_LEFT_BRACKET:
            // rule <else_n> -> { <body> }
            if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                token_error("expected { after else, got %s\n");
                syntax_error();
            }
            check_new_token(EOL_REQUIRED);
            if (semantic_enabled) {
                check_cf(cf_make_if_else_statement(CF_BASIC));
                if ((new_body_table = symtable_init(TABLE_SIZE)) == NULL) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                if (symtable_stack_push(&symtable_stack, new_body_table) == NULL) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                check_cf(cf_assign_statement_symtable(new_body_table));
            }
            check_nonterminal(body());
            if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                token_error("expected } after else body, got %s\n");
                syntax_error();
            }
            if (semantic_enabled) {
                check_cf(cf_pop_previous_branched_statement());
                symtable_stack_pop(&symtable_stack);
            }
            check_new_token(EOL_REQUIRED);
            syntax_ok();
        case TOKEN_KEYWORD:
            if (token.data.keyword_type == KEYWORD_IF) {
                // rule <else_n> -> if expression { <body> } <else>
                if (semantic_enabled) {
                    check_cf(cf_make_if_else_statement(CF_IF));
                }
                check_new_token(EOL_OPTIONAL);
                ASTNode *expression;
                check_nonterminal(parse_expression(PURE_EXPRESSION, true, &expression));
                if (semantic_enabled) {
                    check_cf(cf_use_ast_explicit(expression, CF_IF_CONDITIONAL));
                }
                if (token.context.eol_read) {
                    eol_error("unexpected EOL after if expression\n");
                    syntax_error();
                }
                if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                    token_error("expected { after else if, got %s\n");
                    syntax_error();
                }
                check_new_token(EOL_REQUIRED);
                if (semantic_enabled) {
                    check_cf(cf_make_if_then_statement(CF_BASIC));
                    if ((new_body_table = symtable_init(TABLE_SIZE)) == NULL) {
                        return COMPILER_RESULT_ERROR_INTERNAL;
                    }
                    if (symtable_stack_push(&symtable_stack, new_body_table) == NULL) {
                        return COMPILER_RESULT_ERROR_INTERNAL;
                    }
                    check_cf(cf_assign_statement_symtable(new_body_table));
                }
                check_nonterminal(body());
                if (semantic_enabled) {
                    check_cf(cf_pop_previous_branched_statement());
                    symtable_stack_pop(&symtable_stack);
                }
                if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                    token_error("expected } after else body, got %s\n");
                    syntax_error();
                }
                check_new_token(EOL_OPTIONAL);
                return else_();
            } else {
                token_error("expected if keyword, got %s\n");
                syntax_error();
            }
        default:
            token_error("expected if keyword or { after else keyword, got %s\n");
            syntax_error();
    }
}

int else_() {
    switch (token.type) {
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_RETURN:
                case KEYWORD_IF:
                case KEYWORD_FOR:
                    break;
                case KEYWORD_ELSE:
                    // rule <else> -> else <else_n>
                    if (token.context.eol_read) {
                        eol_error("unexpected EOL after if block\n");
                        syntax_error();
                    }
                    check_new_token(EOL_OPTIONAL);
                    return else_n();
                default:
                    token_error("expected return, if, for or else keyword, got %s\n");
                    syntax_error();
            }
        default:
            // rule <else> -> eps
            // New statement, there must have been EOL
            if (!token.context.eol_read) {
                eol_error("expected EOL after if block before next statement\n");
                syntax_error();
            }
            syntax_ok();
    }
}

int for_definition() {
    ASTNode *definition;
    switch (token.type) {
        case TOKEN_SEMICOLON:
            // rule <for_definition> -> eps
            syntax_ok();
        case TOKEN_ID:
            // rule <for_definition> -> expression
            check_nonterminal(parse_expression(DEFINE_REQUIRED, true, &definition));
            if (semantic_enabled) {
                check_cf(cf_use_ast_explicit(definition, CF_FOR_DEFINITION));
            }
            if (token.context.eol_read) {
                eol_error("unexpected EOL after for definition\n");
                syntax_error();
            }
            syntax_ok();
        default:
            token_error("expected id or semicolon after for, got %s\n");
            syntax_error();
    }
}

int for_assignment() {
    ASTNode *assignment;
    switch (token.type) {
        case TOKEN_CURLY_LEFT_BRACKET:
            // rule <for_assignment> -> eps
            syntax_ok();
        case TOKEN_ID:
            // rule <for_assignment> -> expression
            check_nonterminal(parse_expression(ASSIGN_REQUIRED, false, &assignment));
            if (semantic_enabled) {
                check_cf(cf_use_ast_explicit(assignment, CF_FOR_AFTERTHOUGHT));
            }
            if (token.context.eol_read) {
                eol_error("unexpected EOL after for assignment\n");
                syntax_error();
            }
            syntax_ok();
        default:
            token_error("expected identifier or { in for assignment, got %s\n");
            syntax_error();
    }
}

int return_follow() {
    switch (token.type) {
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_RETURN:
                case KEYWORD_IF:
                case KEYWORD_FOR:
                    if (!token.context.eol_read) {
                        eol_error("expected EOL after return\n");
                    }
                    break;
                default:
                    token_error("expected return, if or for keyword after return, got %s\n");
                    syntax_error();
            }
        case TOKEN_CURLY_RIGHT_BRACKET:
            // rule <return_follow> -> eps
            if (semantic_enabled) {
                ASTNode *emptyList = ast_node_list(0);
                if (emptyList == NULL) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }

                check_cf(cf_use_ast_explicit(emptyList, CF_RETURN_LIST));
            }
            syntax_ok();
        default:
            if (token.context.eol_read) {
                // rule <return_follow> -> eps
                syntax_ok();
            }
            // rule <return_follow> -> expression
            ASTNode *result_node;
            check_nonterminal(parse_expression(PURE_EXPRESSION, true, &result_node));
            if (semantic_enabled) {
                if (result_node->actionType != AST_LIST) {
                    ASTNode *tmp = ast_node_list(1);
                    if (tmp == NULL) {
                        return COMPILER_RESULT_ERROR_INTERNAL;
                    }
                    tmp->data[0].astPtr = result_node;
                    check_cf(cf_use_ast_explicit(tmp, CF_RETURN_LIST));
                } else {
                    check_cf(cf_use_ast_explicit(result_node, CF_RETURN_LIST));
                }
            }
            if (!token.context.eol_read) {
                eol_error("expected EOL after return \n");
                syntax_error();
            }
            syntax_ok();
    }
}

int statement() {
    SymbolTable *new_body_table;
    switch (token.type) {
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_RETURN:
                    // rule <statement> -> return <return_follow>
                    if (semantic_enabled) {
                        check_cf(cf_make_next_statement(CF_RETURN));
                    }
                    check_new_token(EOL_OPTIONAL);
                    return return_follow();
                case KEYWORD_IF:
                    // rule <statement> -> if expression { <body> } <else>
                    check_new_token(EOL_OPTIONAL);
                    ASTNode *result_expr;
                    if (semantic_enabled) {
                        check_cf(cf_make_next_statement(CF_IF));
                    }
                    check_nonterminal(parse_expression(PURE_EXPRESSION, true, &result_expr));
                    if (semantic_enabled) {
                        check_cf(cf_use_ast_explicit(result_expr, CF_IF_CONDITIONAL));
                    }
                    if (token.context.eol_read) {
                        eol_error("unexpected EOL after if expression\n");
                        syntax_error();
                    }
                    if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                        token_error("expected { before if body, got %s\n");
                        syntax_error();
                    }
                    check_new_token(EOL_REQUIRED);
                    if (semantic_enabled) {
                        if ((new_body_table = symtable_init(TABLE_SIZE)) == NULL) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                        if (symtable_stack_push(&symtable_stack, new_body_table) == NULL) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                        check_cf(cf_make_if_then_statement(CF_BASIC));
                        check_cf(cf_assign_statement_symtable(new_body_table));
                    }
                    check_nonterminal(body());
                    if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                        token_error("expected } after if body, got %s\n");
                        syntax_error();
                    }
                    if (semantic_enabled) {
                        check_cf(cf_pop_previous_branched_statement());
                        symtable_stack_pop(&symtable_stack);
                    }
                    check_new_token(EOL_OPTIONAL);
                    return else_();
                case KEYWORD_FOR:
                    // rule <statement> -> for <for_definition> ; expression ; <for_assignment> { <body> }
                    check_new_token(EOL_OPTIONAL);
                    // For definition needs a separate level of symtable.
                    if (semantic_enabled) {
                        check_cf(cf_make_next_statement(CF_FOR));
                        if ((new_body_table = symtable_init(TABLE_SIZE)) == NULL) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                        if (symtable_stack_push(&symtable_stack, new_body_table) == NULL) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                        check_cf(cf_assign_statement_symtable(new_body_table));
                    }
                    check_nonterminal(for_definition());
                    if (token.type != TOKEN_SEMICOLON) {
                        token_error("expected semicolon after for definition, got %s\n");
                        syntax_error();
                    }
                    check_new_token(EOL_OPTIONAL);
                    ASTNode *for_expression;
                    check_nonterminal(parse_expression(PURE_EXPRESSION, false, &for_expression));
                    if (semantic_enabled) {
                        check_cf(cf_use_ast_explicit(for_expression, CF_FOR_CONDITIONAL));
                    }
                    if (token.context.eol_read) {
                        eol_error("unexpected EOL after for expression\n");
                        syntax_error();
                    }
                    if (token.type != TOKEN_SEMICOLON) {
                        token_error("expected semicolon after for condition, got %s\n");
                        syntax_error();
                    }
                    check_new_token(EOL_FORBIDDEN);
                    check_nonterminal(for_assignment());
                    if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                        token_error("expected { before for body, got %s\n");
                        syntax_error();
                    }
                    check_new_token(EOL_REQUIRED);
                    if (semantic_enabled) {
                        check_cf(cf_make_for_body_statement(CF_BASIC));
                        if ((new_body_table = symtable_init(TABLE_SIZE)) == NULL) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                        if (symtable_stack_push(&symtable_stack, new_body_table) == NULL) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                        check_cf(cf_assign_statement_symtable(new_body_table));
                    }
                    check_nonterminal(body());
                    if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                        token_error("expected } after for body, got %s\n");
                        syntax_error();
                    }
                    if (semantic_enabled) {
                        check_cf(cf_pop_previous_branched_statement());
                        symtable_stack_pop(&symtable_stack);
                        symtable_stack_pop(&symtable_stack);
                    }
                    check_new_token(EOL_REQUIRED);
                    syntax_ok();
                default:
                    token_error("expected identifier, for, if or return at statement start, got %s\n");
                    syntax_error();
            }
        case TOKEN_ID:
            // rule <statement> -> expression
            check_cf(cf_make_next_statement(CF_BASIC));
            ASTNode *expression;
            check_nonterminal(parse_expression(VALID_STATEMENT, true, &expression));
            check_cf(cf_use_ast_explicit(expression, CF_STATEMENT_BODY));
            if (!token.context.eol_read) {
                eol_error("expected EOL after expression\n");
                syntax_error();
            }
            syntax_ok();
        default:
            token_error("expected identifier, for, if or return at statement start, got %s\n");
            syntax_error();
    }
}

int body() {
    switch (token.type) {
        case TOKEN_CURLY_RIGHT_BRACKET:
            // rule <body> -> eps
            syntax_ok();
        case TOKEN_KEYWORD:
            // rule <body> -> <statement> <body>
            switch (token.data.keyword_type) {
                case KEYWORD_RETURN:
                case KEYWORD_IF:
                case KEYWORD_FOR:
                    check_nonterminal(statement());
                    return body();
                default:
                    token_error("expected }, identifier, for, if or return at function body start, got %s\n");
                    syntax_error();
            }
        case TOKEN_ID:
            // rule <body> -> <statement> <body>
            check_nonterminal(statement());
            return body();
        default:
            token_error("expected }, identifier, for, if or return at function body start, got %s\n");
            syntax_error();
    }
}

int ret_type_n(STItem *current_function) {
    STDataType data_type;
    switch (token.type) {
        case TOKEN_RIGHT_BRACKET:
            // rule <ret_type_n> -> eps
            syntax_ok();
        case TOKEN_COMMA:
            // rule <ret_type_n> -> , <type> <ret_type_n>
            check_new_token(EOL_OPTIONAL);
            check_nonterminal(type(&data_type));
            if (semantic_enabled) {
                check_cf(cf_add_return_value(NULL, data_type));
                if (!symtable_add_ret_type(current_function, NULL, data_type)) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
            }
            return ret_type_n(current_function);
        default:
            token_error("expected comma or ) after type inside return type, got %s\n");
            syntax_error();
    }
}

int ret_type_inner(STItem *current_function) {
    STDataType data_type;
    switch (token.type) {
        case TOKEN_ID:
        case TOKEN_RIGHT_BRACKET:
            // rule <ret_type_inner> -> <params>
            return params(current_function, true, false);
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_FLOAT64:
                case KEYWORD_INT:
                case KEYWORD_STRING:
                case KEYWORD_BOOL:
                    // rule <ret_type_inner> -> <type> <ret_type_n>
                    check_nonterminal(type(&data_type));
                    if (semantic_enabled) {
                        check_cf(cf_add_return_value(NULL, data_type));
                        if (!symtable_add_ret_type(current_function, NULL, data_type)) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                    }
                    return ret_type_n(current_function);
                default:
                    token_error("col %u: expected float64, int, string or bool keyword, got %s\n");
                    syntax_error();
            }
        default:
            token_error("expected identifier, ), float64, int, string or bool in return type, got %s\n");
            syntax_error();
    }
}

int ret_type(STItem *current_function) {
    STDataType data_type;
    switch (token.type) {
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_FLOAT64:
                case KEYWORD_INT:
                case KEYWORD_STRING:
                case KEYWORD_BOOL:
                    // rule <ret_type> -> <type>
                    check_nonterminal(type(&data_type));
                    if (semantic_enabled) {
                        check_cf(cf_add_return_value(NULL, data_type));
                        if (!symtable_add_ret_type(current_function, NULL, data_type)) {
                            return COMPILER_RESULT_ERROR_INTERNAL;
                        }
                    }
                    syntax_ok();
                default:
                    token_error("expected float64, int, string or bool keyword, got %s\n");
                    syntax_error();
            }
        case TOKEN_CURLY_LEFT_BRACKET:
            // rule <ret_type> -> eps
            syntax_ok();
        case TOKEN_LEFT_BRACKET:
            // rule <ret_type> -> ( <ret_type_inner> )
            check_new_token(EOL_OPTIONAL);
            check_nonterminal(ret_type_inner(current_function));
            if (token.type != TOKEN_RIGHT_BRACKET) {
                token_error("expected ) after multiple function return types, got %s\n");
                syntax_error();
            }
            check_new_token(EOL_FORBIDDEN);
            syntax_ok();
        default:
            token_error("expected {, ( or type keyword at the start of return type, got %s\n");
            syntax_error();
    }
}

int execution() {
    if (scanner_result == SCANNER_RESULT_EOF) {
        // Found EOF, simulate rule <execution> -> eps
        syntax_ok();
    } else {
        // rule <execution> -> func id ( <params> ) <ret_type> { <body> } <execution>
        if (token.type != TOKEN_KEYWORD || token.data.keyword_type != KEYWORD_FUNC) {
            token_error("expected func keyword at the start of function definition, got %s\n");
            syntax_error();
        }

        check_new_token(EOL_FORBIDDEN);
        if (token.type != TOKEN_ID) {
            token_error("expected function identifier after func keyword, got %s\n");
            syntax_error();
        }

        STItem *function = symtable_find(function_table, mstr_content(&token.data.str_val));
        bool already_found = false;
        if (semantic_enabled) {
            if (function) {
                if (function->data.data.func_data.defined) {
                    redefine_error("redefinition of function %s\n");
                    semantic_error_redefine();
                } else {
                    already_found = true;
                }
            } else {
                function = symtable_add(function_table, mstr_content(&token.data.str_val), ST_SYMBOL_FUNC);
                if (function == NULL) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
            }
            function->data.data.func_data.defined = true;
        }

        if (semantic_enabled) {
            check_cf(cf_make_function(mstr_content(&token.data.str_val)));
        }

        clear_token();
        check_new_token(EOL_FORBIDDEN);
        if (token.type != TOKEN_LEFT_BRACKET) {
            token_error("expected ( after function identifier, got %s\n");
            syntax_error();
        }

        if (semantic_enabled) {
            SymbolTable *body_table = symtable_init(TABLE_SIZE);
            check_cf(cf_assign_function_symtable(body_table));
            if (body_table == NULL || symtable_stack_push(&symtable_stack, body_table) == NULL) {
                return COMPILER_RESULT_ERROR_INTERNAL;
            }
        }
        check_new_token(EOL_OPTIONAL);
        check_nonterminal(params(function, false, already_found));

        if (token.type != TOKEN_RIGHT_BRACKET) {
            token_error("expected ) after function parameters, got %s\n");
            syntax_error();
        }

        check_new_token(EOL_FORBIDDEN);
        check_nonterminal(ret_type(function));

        if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
            token_error("expected { after function return type, got %s\n");
            syntax_error();
        }

        check_new_token(EOL_REQUIRED);
        check_nonterminal(body());
        if (semantic_enabled) {
            symtable_stack_pop(&symtable_stack);
        }

        if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
            token_error("expected } after function body, got %s\n");
            syntax_error();
        }

        // Recursively check <execution>, new func must be on a new line.
        check_new_token(EOL_REQUIRED);
        return execution();
    }
    syntax_error();
}

bool prepare_builtins() {
    STItem *inputs = symtable_add(function_table, "inputs", ST_SYMBOL_FUNC);
    if (inputs == NULL) {
        return false;
    }
    inputs->data.data.func_data.defined = true;
    if (!symtable_add_ret_type(inputs, NULL, CF_STRING) || !symtable_add_ret_type(inputs, NULL, CF_INT)) {
        return false;
    }
    STItem *inputi = symtable_add(function_table, "inputi", ST_SYMBOL_FUNC);
    if (inputi == NULL) {
        return false;
    }
    inputi->data.data.func_data.defined = true;
    if (!symtable_add_ret_type(inputi, NULL, CF_INT) || !symtable_add_ret_type(inputi, NULL, CF_INT)) {
        return false;
    }
    STItem *inputf = symtable_add(function_table, "inputf", ST_SYMBOL_FUNC);
    if (inputf == NULL) {
        return false;
    }
    inputf->data.data.func_data.defined = true;
    if (!symtable_add_ret_type(inputf, NULL, CF_FLOAT) || !symtable_add_ret_type(inputf, NULL, CF_INT)) {
        return false;
    }
    STItem *print = symtable_add(function_table, "print", ST_SYMBOL_FUNC);
    if (print == NULL) {
        return false;
    }
    print->data.data.func_data.defined = true;
    // Do not add any print arguments, this will be taken care of by AST.
    STItem *int2float = symtable_add(function_table, "int2float", ST_SYMBOL_FUNC);
    if (int2float == NULL) {
        return false;
    }
    int2float->data.data.func_data.defined = true;
    if (!symtable_add_param(int2float, "i", CF_INT) || !symtable_add_ret_type(int2float, NULL, CF_FLOAT)) {
        return false;
    }
    STItem *float2int = symtable_add(function_table, "float2int", ST_SYMBOL_FUNC);
    if (float2int == NULL) {
        return false;
    }
    float2int->data.data.func_data.defined = true;
    if (!symtable_add_param(float2int, "i", CF_FLOAT) || !symtable_add_ret_type(float2int, NULL, CF_INT)) {
        return false;
    }
    STItem *len = symtable_add(function_table, "len", ST_SYMBOL_FUNC);
    if (len == NULL) {
        return false;
    }
    len->data.data.func_data.defined = true;
    if (!symtable_add_param(len, "s", CF_STRING) || !symtable_add_ret_type(len, NULL, CF_INT)) {
        return false;
    }
    STItem *substr = symtable_add(function_table, "substr", ST_SYMBOL_FUNC);
    if (substr == NULL) {
        return false;
    }
    substr->data.data.func_data.defined = true;
    if (!symtable_add_param(substr, "s", CF_STRING) || !symtable_add_param(substr, "i", CF_INT) ||
        !symtable_add_param(substr, "n", CF_INT) || !symtable_add_ret_type(substr, NULL, CF_STRING) ||
        !symtable_add_ret_type(substr, NULL, CF_INT)) {
        return false;
    }
    STItem *ord = symtable_add(function_table, "ord", ST_SYMBOL_FUNC);
    if (ord == NULL) {
        return false;
    }
    ord->data.data.func_data.defined = true;
    if (!symtable_add_param(ord, "s", CF_STRING) || !symtable_add_param(ord, "i", CF_INT) ||
        !symtable_add_ret_type(ord, NULL, CF_INT) || !symtable_add_ret_type(ord, NULL, CF_INT)) {
        return false;
    }
    STItem *chr = symtable_add(function_table, "chr", ST_SYMBOL_FUNC);
    if (chr == NULL) {
        return false;
    }
    chr->data.data.func_data.defined = true;
    if (!symtable_add_param(chr, "i", CF_INT) || !symtable_add_ret_type(chr, NULL, CF_STRING) ||
        !symtable_add_ret_type(chr, NULL, CF_INT)) {
        return false;
    }
    return true;
}

int program() {
    // rule <program> -> package id <execution>
    function_table = symtable_init(TABLE_SIZE);
    if (function_table == NULL) {
        return COMPILER_RESULT_ERROR_INTERNAL;
    }
    check_cf(cf_assign_global_symtable(function_table));
    if (!prepare_builtins()) {
        return COMPILER_RESULT_ERROR_INTERNAL;
    }

    if (token.type != TOKEN_KEYWORD || token.data.keyword_type != KEYWORD_PACKAGE) {
        token_error("expected package keyword at the beginning of file, got %s\n");
        syntax_error();
    }
    check_new_token(EOL_OPTIONAL);
    if (token.type != TOKEN_ID || strcmp("main", mstr_content(&token.data.str_val)) != 0) {
        token_error("expected main identifier after package keyword, got %s\n");
        syntax_error();
    }
    clear_token();
    check_new_token(EOL_REQUIRED);
    check_nonterminal(execution());
    if (semantic_enabled) {
        STItem *main = symtable_find(function_table, "main");
        if (main == NULL || !main->data.data.func_data.defined) {
            stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE,
                           "missing function main\n");
            semantic_error_redefine();
        } else if (main->data.data.func_data.ret_types != NULL || main->data.data.func_data.params != NULL) {
            stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,
                           "incorrect prototype of function main\n");
            return COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE;
        }
        main->data.reference_counter = 1;
        for (STItem *function = symtable_get_first_item(function_table); function != NULL;
             function = symtable_get_next_item(function_table, function)) {
            if (!function->data.data.func_data.defined) {
                stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE,
                               "undefined function %s\n", function->key);
                return COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE;
            }
        }
    }
    syntax_ok();
}

CompilerResult parser_parse() {
    check_cf(cf_init());
    symtable_stack_init(&symtable_stack);
    check_new_token(EOL_OPTIONAL);
    return program();
}
