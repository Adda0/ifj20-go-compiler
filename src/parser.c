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


int body() {
    switch (token.type) {
        case TOKEN_CURLY_RIGHT_BRACKET:
            // rule <body> -> eps
            syntax_ok();
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
