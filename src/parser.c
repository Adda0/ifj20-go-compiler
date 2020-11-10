/** @file parser.c
 *
 * IFJ20 compiler
 *
 * @brief Contains definitions of functions and data types for IFJ20 parser.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "compiler.h"
#include "scanner.h"
#include "mutable_string.h"

Token token;
ScannerResult scanner_result;

int body();
int else_();

void clear_token() {
    if (token.type == TOKEN_ID || token.type == TOKEN_STRING) {
        mstr_free(&token.data.str_val);
    }
}

int expression(EolRule eol) {
    // TODO: temporary solution to allow simple terms instead of precedence expression parsing. Dirty hack
    switch (token.type) {
        case TOKEN_BOOL:
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_STRING:
        case TOKEN_ID:
            clear_token();
            check_new_token(eol);
            syntax_ok();
        default:
            syntax_error();
    }
}

int type() {
    if (token.type != TOKEN_KEYWORD) {
        syntax_error();
    }
    switch (token.data.keyword_type) {
        case KEYWORD_FLOAT64: // rule <type> -> float64
        case KEYWORD_INT:     // rule <type> -> int
        case KEYWORD_STRING:  // rule <type> -> string
        case KEYWORD_BOOL:    // rule <type> -> bool
            // We consumed one token, get a new one and return OK
            check_new_token(EOL_FORBIDDEN);
            syntax_ok();
        default:
            syntax_error();
    }
}

int params_n() {
    switch (token.type) {
        case TOKEN_RIGHT_BRACKET:
            // rule <params_n> -> eps
            syntax_ok();
        case TOKEN_COMMA:
            // rule <params_n> -> , id <type> <params_n>
            // we've read comma, newline is allowed according to FUNEXP
            check_new_token(EOL_OPTIONAL);
            if (token.type != TOKEN_ID) {
                syntax_error();
            }
            clear_token();

            check_new_token(EOL_FORBIDDEN);
            check_nonterminal(type());
            return params_n();
        default:
            syntax_error();
    }
}

int params() {
    switch (token.type) {
        case TOKEN_RIGHT_BRACKET:
            // rule <params> -> eps
            syntax_ok();
        case TOKEN_ID:
            // rule <params> -> id <type> <params_n>
            clear_token();
            check_new_token(EOL_FORBIDDEN);
            check_nonterminal(type());
            return params_n();
        default:
            syntax_error();
    }
}

int expression_n() {
    switch (token.type) {
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_RETURN:
                case KEYWORD_IF:
                case KEYWORD_FOR:
                    break;
                default:
                    syntax_error();
            }
        case TOKEN_ID:
        case TOKEN_RIGHT_BRACKET:
        case TOKEN_CURLY_LEFT_BRACKET:
        case TOKEN_CURLY_RIGHT_BRACKET:
        case TOKEN_SEMICOLON:
            // rule <expression_n> -> eps
            syntax_ok();
        case TOKEN_COMMA:
            // rule <expression_n> -> , expression <expression_n>
            //TODO: expression
            check_new_token(EOL_OPTIONAL);
            check_nonterminal(expression(EOL_OPTIONAL));
            return expression_n();
        default:
            syntax_error();
    }
}

int call_params() {
    switch (token.type) {
        case TOKEN_RIGHT_BRACKET:
            // rule <call_params> -> eps
            syntax_ok();
        case TOKEN_BOOL:
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_STRING:
        case TOKEN_ID:
            // rule <call_params> -> expression <expression_n>
            // TODO: expression
            check_nonterminal(expression(EOL_FORBIDDEN));
            return expression_n();
        default:
            syntax_error();
    }
}

int id_n() {
    switch (token.type) {
        case TOKEN_COMMA:
            // rule <id_n> -> , id <id_n>
            check_new_token(EOL_OPTIONAL);
            if (token.type != TOKEN_ID) {
                syntax_error();
            }
            clear_token();
            check_new_token(EOL_FORBIDDEN);
            return id_n();
        case TOKEN_ASSIGN:
        case TOKEN_DEFINE:
            // rule <id_n> -> eps
            syntax_ok();
        default:
            syntax_error();
    }
}

int assignment() {
    switch (token.type) {
        case TOKEN_ASSIGN:
            // rule <assignment> -> = expression <expression_n>
            // TODO: expression
            check_new_token(EOL_OPTIONAL);
            check_nonterminal(expression(EOL_OPTIONAL));
            return expression_n();
        case TOKEN_DEFINE:
            // rule <assignment> -> := expression <expression_n>
            // TODO: expression
            check_new_token(EOL_OPTIONAL);
            check_nonterminal(expression(EOL_OPTIONAL));
            return expression_n();
        default:
            syntax_error();
    }
}

int unary() {
    switch (token.type) {
        case TOKEN_PLUS_ASSIGN:     // rule <unary> -> += expression
        case TOKEN_MINUS_ASSIGN:    // rule <unary> -> -= expression
        case TOKEN_MULTIPLY_ASSIGN: // rule <unary> -> *= expression
        case TOKEN_DIVIDE_ASSIGN:   // rule <unary> -> /= expression
            // TODO: expression
            check_new_token(EOL_OPTIONAL);
            check_nonterminal(expression(EOL_OPTIONAL));
            syntax_ok();
        default:
            syntax_error();
    }
}

int id_follow() {
    switch (token.type) {
        case TOKEN_LEFT_BRACKET:
            // rule <id_follow> -> ( <call_params> )
            if (token.type != TOKEN_LEFT_BRACKET) {
                syntax_error();
            }
            check_new_token(EOL_FORBIDDEN);
            check_nonterminal(call_params());
            if (token.type != TOKEN_RIGHT_BRACKET) {
                syntax_error();
            }
            // Function call on its own line, there needs to be a new line
            check_new_token(EOL_REQUIRED);
            syntax_ok();
        case TOKEN_COMMA:
            // rule <id_follow> -> <id_n> <assignment>
            check_nonterminal(id_n());
            return assignment();
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MULTIPLY_ASSIGN:
        case TOKEN_DIVIDE_ASSIGN:
            // rule <id_follow> -> <unary>
            return unary();
        case TOKEN_ASSIGN:
        case TOKEN_DEFINE:
            // rule <id_follow> -> <id_n> <assignment>
            check_nonterminal(id_n());
            return assignment();
        default:
            syntax_error();
    }
}

int else_n() {
    switch (token.type) {
        case TOKEN_CURLY_LEFT_BRACKET:
            // rule <else_n> -> { <body> }
            if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                syntax_error();
            }
            check_new_token(EOL_REQUIRED);
            check_nonterminal(body());
            if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                syntax_error();
            }
            syntax_ok();
        case TOKEN_KEYWORD:
            if (token.data.keyword_type == KEYWORD_IF) {
                // rule <else_n> -> if expression { <body> } <else>
                // TODO: expression
                check_new_token(EOL_FORBIDDEN);
                check_nonterminal(expression(EOL_OPTIONAL));
                if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                    syntax_error();
                }
                check_new_token(EOL_REQUIRED);
                check_nonterminal(body());
                if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                    syntax_error();
                }
                check_new_token(EOL_FORBIDDEN);
                return else_();
            } else {
                syntax_error();
            }
        default:
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
                    check_new_token(EOL_FORBIDDEN);
                    return else_n();
                default:
                    syntax_error();
            }
        case TOKEN_ID:
        case TOKEN_CURLY_RIGHT_BRACKET:
            // rule <else> -> eps
            syntax_ok();
        default:
            syntax_error();
    }
}

int for_definition() {
    switch (token.type) {
        case TOKEN_SEMICOLON:
            // rule <for_definition> -> eps
            syntax_ok();
        case TOKEN_ID:
            // rule <for_definition> -> id <id_n> := expression <expression_n>
            clear_token();
            check_new_token(EOL_FORBIDDEN);
            check_nonterminal(id_n());
            if (token.type != TOKEN_DEFINE) {
                syntax_error();
            }
            // TODO: epxression
            check_new_token(EOL_FORBIDDEN);
            check_nonterminal(expression(EOL_FORBIDDEN));
            return expression_n();
        default:
            syntax_error();
    }
}

int for_assignment_follow() {
    switch (token.type) {
        case TOKEN_COMMA:
        case TOKEN_ASSIGN:
            // rule <for_assignment_follow> -> <id_n> = expression <expression_n>
            check_nonterminal(id_n());
            if (token.type != TOKEN_ASSIGN) {
                syntax_error();
            }
            // TODO: expression
            check_new_token(EOL_OPTIONAL);
            check_nonterminal(expression(EOL_FORBIDDEN));
            return expression_n();
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MULTIPLY_ASSIGN:
        case TOKEN_DIVIDE_ASSIGN:
            // <for_assignment_follow> -> <unary>
            return unary();
        default:
            syntax_error();
    }
}

int for_assignment() {
    switch (token.type) {
        case TOKEN_ID:
            // rule <for_assignment> -> id <for_assignemnt_follow>
            clear_token();
            check_new_token(EOL_FORBIDDEN);
            return for_assignment_follow();
        case TOKEN_CURLY_LEFT_BRACKET:
            // rule <for_assignment> -> eps
            syntax_ok();
        default:
            syntax_error();
    }
}

int statement() {
    switch (token.type) {
        case TOKEN_ID:
            // rule <statement> -> id <id_follow>
            clear_token();
            check_new_token(EOL_FORBIDDEN);
            return id_follow();
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_RETURN:
                    // rule <statement> -> return expression <expression_n>
                    // TODO: expression
                    check_new_token(EOL_FORBIDDEN);
                    check_nonterminal(expression(EOL_OPTIONAL));
                    return expression_n();
                case KEYWORD_IF:
                    // rule <statement> -> if expression { <body> } <else>
                    // TODO: expression
                    check_new_token(EOL_OPTIONAL);
                    check_nonterminal(expression(EOL_FORBIDDEN));
                    if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                        syntax_error();
                    }
                    check_new_token(EOL_REQUIRED);
                    check_nonterminal(body());
                    if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                        syntax_error();
                    }
                    check_new_token(EOL_REQUIRED);
                    return else_();
                case KEYWORD_FOR:
                    // rule <statement> -> for <for_definition> ; expression ; <for_assignment> { <body> }
                    check_new_token(EOL_OPTIONAL);
                    check_nonterminal(for_definition());
                    if (token.type != TOKEN_SEMICOLON) {
                        syntax_error();
                    }
                    // TODO: expression
                    check_new_token(EOL_OPTIONAL);
                    check_nonterminal(expression(EOL_FORBIDDEN));
                    if (token.type != TOKEN_SEMICOLON) {
                        syntax_error();
                    }
                    check_new_token(EOL_FORBIDDEN);
                    check_nonterminal(for_assignment());
                    if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                        syntax_error();
                    }
                    check_new_token(EOL_REQUIRED);
                    check_nonterminal(body());
                    if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                        syntax_error();
                    }
                    check_new_token(EOL_REQUIRED);
                    syntax_ok();
                default:
                    syntax_error();
            }
        default:
            syntax_error();
    }
}

int body() {
    switch (token.type) {
        case TOKEN_CURLY_RIGHT_BRACKET:
            // rule <body> -> eps
            syntax_ok();
        case TOKEN_ID:
            // rule <body> -> <statement> <body>
            check_nonterminal(statement());
            return body();
        case TOKEN_KEYWORD:
            // rule <body> -> <statement> <body>
            switch (token.data.keyword_type) {
                case KEYWORD_RETURN:
                case KEYWORD_IF:
                case KEYWORD_FOR:
                    check_nonterminal(statement());
                    return body();
                default:
                    syntax_error();
            }
        default:
            syntax_error();
    }
}

int ret_type_n() {
    switch (token.type) {
        case TOKEN_RIGHT_BRACKET:
            // rule <ret_type_n> -> eps
            syntax_ok();
        case TOKEN_COMMA:
            // rule <ret_type_n> -> , <type> <ret_type_n>
            check_new_token(EOL_OPTIONAL);
            check_nonterminal(type());
            return ret_type_n();
        default:
            syntax_error();
    }
}

int ret_type_inner() {
    switch (token.type) {
        case TOKEN_ID:
        case TOKEN_RIGHT_BRACKET:
            // rule <ret_type_inner> -> <params>
            return params();
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_FLOAT64:
                case KEYWORD_INT:
                case KEYWORD_STRING:
                case KEYWORD_BOOL:
                    // rule <ret_type_inner> -> <type> <ret_type_n>
                    check_nonterminal(type());
                    return ret_type_n();
                default:
                    syntax_error();
            }
        default:
            syntax_error();
    }
}

int ret_type() {
    switch (token.type) {
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_FLOAT64:
                case KEYWORD_INT:
                case KEYWORD_STRING:
                case KEYWORD_BOOL:
                    // rule <ret_type> -> <type>
                    return type();
                default:
                    syntax_error();
            }
        case TOKEN_CURLY_LEFT_BRACKET:
            // rule <ret_type> -> eps
            syntax_ok();
        case TOKEN_LEFT_BRACKET:
            // rule <ret_type> -> ( <ret_type_inner> )
            check_new_token(EOL_FORBIDDEN);
            check_nonterminal(ret_type_inner());
            if (token.type != TOKEN_RIGHT_BRACKET) {
                syntax_error();
            }
            check_new_token(EOL_FORBIDDEN);
            syntax_ok();
        default:
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
            syntax_error();
        }

        check_new_token(EOL_FORBIDDEN);
        if (token.type != TOKEN_ID) {
            syntax_error();
        }
        clear_token();

        check_new_token(EOL_FORBIDDEN);
        if (token.type != TOKEN_LEFT_BRACKET) {
            syntax_error();
        }

        check_new_token(EOL_FORBIDDEN);
        check_nonterminal(params());

        if (token.type != TOKEN_RIGHT_BRACKET) {
            syntax_error();
        }

        check_new_token(EOL_FORBIDDEN);
        check_nonterminal(ret_type());

        if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
            syntax_error();
        }

        check_new_token(EOL_REQUIRED);
        check_nonterminal(body());

        if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
            syntax_error();
        }

        // Recursively check <execution>, new func must be on a new line.
        check_new_token(EOL_REQUIRED);
        return execution();
    }
    syntax_error();
}

int program() {
    // rule <program> -> package id <execution>
    if (token.type != TOKEN_KEYWORD || token.data.keyword_type != KEYWORD_PACKAGE) {
        syntax_error();
    }
    check_new_token(EOL_FORBIDDEN);
    if (token.type != TOKEN_ID || strcmp("main", mstr_content(&token.data.str_val)) != 0) {
        syntax_error();
    }
    clear_token();
    check_new_token(EOL_OPTIONAL);
    return execution();
}

CompilerResult parser_parse() {
    check_new_token(EOL_OPTIONAL);
    return program();
}
