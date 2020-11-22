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

Token token, prev_token;
ScannerResult scanner_result;

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
        case TOKEN_DEFAULT: return "undefined";
        case TOKEN_ID: return "identifier";
        case TOKEN_INT: return "int value";
        case TOKEN_FLOAT: return "float value";
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_BOOL: return "keyword bool";
                case KEYWORD_ELSE: return "keyword else";
                case KEYWORD_FLOAT64: return "keyword float64";
                case KEYWORD_FOR: return "keyword for";
                case KEYWORD_FUNC: return "keyword func";
                case KEYWORD_IF: return "keyword if";
                case KEYWORD_INT: return "keyword int";
                case KEYWORD_PACKAGE: return "keyword package";
                case KEYWORD_RETURN: return "keyword return";
                case KEYWORD_STRING: return "keyword string";
                default: return "";
            }
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_MULTIPLY: return "*";
        case TOKEN_DIVIDE: return "/";
        case TOKEN_PLUS_ASSIGN: return "+=";
        case TOKEN_MINUS_ASSIGN: return "-=";
        case TOKEN_MULTIPLY_ASSIGN: return "*=";
        case TOKEN_DIVIDE_ASSIGN: return "/=";
        case TOKEN_DEFINE: return ":=";
        case TOKEN_ASSIGN: return "=";
        case TOKEN_EQUAL_TO: return "==";
        case TOKEN_BOOL: return "bool value";
        case TOKEN_NOT: return "!";
        case TOKEN_NOT_EQUAL_TO: return "!=";
        case TOKEN_AND: return "&&";
        case TOKEN_OR: return "||";
        case TOKEN_LEFT_BRACKET: return "(";
        case TOKEN_RIGHT_BRACKET: return ")";
        case TOKEN_CURLY_LEFT_BRACKET: return "{";
        case TOKEN_CURLY_RIGHT_BRACKET: return "}";
        case TOKEN_LESS_THAN: return "<";
        case TOKEN_GREATER_THAN: return ">";
        case TOKEN_LESS_OR_EQUAL: return "<=";
        case TOKEN_GREATER_OR_EQUAL: return ">=";
        case TOKEN_STRING: return "string value";
        case TOKEN_COMMA: return ",";
        case TOKEN_SEMICOLON: return ";";
        default: return "";
    }
}

void clear_token() {
    if (token.type == TOKEN_ID || token.type == TOKEN_STRING) {
        mstr_free(&token.data.str_val);
    }
}

int type() {
    if (token.type != TOKEN_KEYWORD) {
        token_error("expected float64, int, string or bool keyword, got %s\n");
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
            token_error("expected float64, int, string or bool keyword, got %s\n");
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
                token_error("expected identifier, got %s\n");
                syntax_error();
            }
            clear_token();

            check_new_token(EOL_FORBIDDEN);
            check_nonterminal(type());
            return params_n();
        default:
            token_error("expected ) or , when parsing parameters, got %s\n");
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
            token_error("expected ) or identifier when parsing parameters, got %s\n");
            syntax_error();
    }
}

int else_n() {
    switch (token.type) {
        case TOKEN_CURLY_LEFT_BRACKET:
            // rule <else_n> -> { <body> }
            if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                token_error("expected { after else, got %s\n");
                syntax_error();
            }
            check_new_token(EOL_REQUIRED);
            check_nonterminal(body());
            if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                token_error("expected } after else body, got %s\n");
                syntax_error();
            }
            check_new_token(EOL_REQUIRED);
            syntax_ok();
        case TOKEN_KEYWORD:
            if (token.data.keyword_type == KEYWORD_IF) {
                // rule <else_n> -> if expression { <body> } <else>
                check_new_token(EOL_OPTIONAL);
                check_nonterminal(parse_expression(PURE_EXPRESSION, true));
                if (token.context.eol_read) {
                    eol_error("unexpected EOL after if expression\n");
                    syntax_error();
                }
                if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                    token_error("expected { after else if, got %s\n");
                    syntax_error();
                }
                check_new_token(EOL_REQUIRED);
                check_nonterminal(body());
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
    switch (token.type) {
        case TOKEN_SEMICOLON:
            // rule <for_definition> -> eps
            syntax_ok();
        case TOKEN_ID:
            // rule <for_definition> -> expression
            check_nonterminal(parse_expression(DEFINE_REQUIRED, false));
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
    switch (token.type) {
        case TOKEN_CURLY_LEFT_BRACKET:
            // rule <for_assignment> -> eps
            syntax_ok();
        case TOKEN_ID:
            // rule <for_assignment> -> expression
            check_nonterminal(parse_expression(ASSIGN_REQUIRED, false));
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
            syntax_ok();
        default:
            if (token.context.eol_read) {
                // rule <return_follow> -> eps
                syntax_ok();
            }
            // rule <return_follow> -> expression
            check_nonterminal(parse_expression(PURE_EXPRESSION, true));
            if (!token.context.eol_read) {
                eol_error("expected EOL after return \n");
                syntax_error();
            }
            syntax_ok();
    }
}

int statement() {
    switch (token.type) {
        case TOKEN_KEYWORD:
            switch (token.data.keyword_type) {
                case KEYWORD_RETURN:
                    // rule <statement> -> return <return_follow>
                    check_new_token(EOL_OPTIONAL);
                    return return_follow();
                case KEYWORD_IF:
                    // rule <statement> -> if expression { <body> } <else>
                    check_new_token(EOL_OPTIONAL);
                    check_nonterminal(parse_expression(PURE_EXPRESSION, true));
                    if (token.context.eol_read) {
                        eol_error("unexpected EOL after if expression\n");
                        syntax_error();
                    }
                    if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
                        token_error("expected { before if body, got %s\n");
                        syntax_error();
                    }
                    check_new_token(EOL_REQUIRED);
                    check_nonterminal(body());
                    if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                        token_error("expected } after if body, got %s\n");
                        syntax_error();
                    }
                    check_new_token(EOL_OPTIONAL);
                    return else_();
                case KEYWORD_FOR:
                    // rule <statement> -> for <for_definition> ; expression ; <for_assignment> { <body> }
                    check_new_token(EOL_OPTIONAL);
                    check_nonterminal(for_definition());
                    if (token.type != TOKEN_SEMICOLON) {
                        token_error("expected semicolon after for definition, got %s\n");
                        syntax_error();
                    }
                    check_new_token(EOL_OPTIONAL);
                    check_nonterminal(parse_expression(PURE_EXPRESSION, false));
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
                    check_nonterminal(body());
                    if (token.type != TOKEN_CURLY_RIGHT_BRACKET) {
                        token_error("expected } after for body, got %s\n");
                        syntax_error();
                    }
                    check_new_token(EOL_REQUIRED);
                    syntax_ok();
                default:
                    token_error("expected identifier, for, if or return at statement start, got %s\n");
                    syntax_error();
            }
        case TOKEN_ID:
            // rule <statement> -> expression
            check_nonterminal(parse_expression(VALID_STATEMENT, true));
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
            token_error("expected comma or ) after type inside return type, got %s\n");
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
                    token_error("col %u: expected float64, int, string or bool keyword, got %s\n");
                    syntax_error();
            }
        default:
            token_error("expected identifier, ), float64, int, string or bool in return type, got %s\n");
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
                    token_error("expected float64, int, string or bool keyword, got %s\n");
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
        clear_token();

        check_new_token(EOL_FORBIDDEN);
        if (token.type != TOKEN_LEFT_BRACKET) {
            token_error("expected ( after function identifier, got %s\n");
            syntax_error();
        }

        check_new_token(EOL_FORBIDDEN);
        check_nonterminal(params());

        if (token.type != TOKEN_RIGHT_BRACKET) {
            token_error("expected ) after function parameters, got %s\n");
            syntax_error();
        }

        check_new_token(EOL_FORBIDDEN);
        check_nonterminal(ret_type());

        if (token.type != TOKEN_CURLY_LEFT_BRACKET) {
            token_error("expected { after function return type, got %s\n");
            syntax_error();
        }

        check_new_token(EOL_REQUIRED);
        check_nonterminal(body());

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

int program() {
    // rule <program> -> package id <execution>
    if (token.type != TOKEN_KEYWORD || token.data.keyword_type != KEYWORD_PACKAGE) {
        token_error("expected package keyword at the beginning of file, got %s\n");
        syntax_error();
    }
    check_new_token(EOL_FORBIDDEN);
    if (token.type != TOKEN_ID || strcmp("main", mstr_content(&token.data.str_val)) != 0) {
        token_error("expected main identifier after package keyword, got %s\n");
        syntax_error();
    }
    clear_token();
    check_new_token(EOL_REQUIRED);
    return execution();
}

CompilerResult parser_parse() {
    check_new_token(EOL_OPTIONAL);
    return program();
}
