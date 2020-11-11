/** @file scanner.c
 *
 * IFJ20 compiler
 *
 * @brief Implements scanner
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <float.h>

#include "scanner.h"
#include "scanner_static.h"
#include "stderr_message.h"


ScannerResult scanner_get_token(Token *token, EolRule eol_rule) {
    // 'static' solves the problem of reading one more char before returning Token
    static char read_char = EMPTY_CHAR; // the last read character from the source code
    static NextCharResult next_char_result = NEXT_CHAR_RESULT_SUCCESS; // result returned when reading read_char
    static size_t line_num = 0; // number of current line
    static size_t char_num = 0; // number of current char in a line

    EolRuleResult eol_rule_result = EOL_RULE_RESULT_SUCCESS;

    token->type = TOKEN_DEFAULT;
    bool token_done = false; // Whether the token is ready to be send to parser. Simulates accept state in FA.

    AutomatonState automaton_state = STATE_DEFAULT;
    ScannerResult scanner_result = SCANNER_RESULT_SUCCESS;
        if (read_char == EMPTY_CHAR) { // get new character from source code
            next_char_result = get_next_char(&read_char);

            if (next_char_result == NEXT_CHAR_RESULT_ERROR) {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL, "Reading a next character from the source code failed.");
                mstr_free(&mutable_string);
                return SCANNER_RESULT_INTERNAL_ERROR;
            }

            if (next_char_result == NEXT_CHAR_RESULT_EOF) {
                scanner_result = SCANNER_RESULT_EOF;
            }
        }
        read_char = resolve_read_char(read_char, line_num, char_num, &automaton_state, &scanner_result, &mutable_string, token, &token_done);
    return scanner_result;
}

static char resolve_read_char(char read_char, size_t line_num, size_t char_num, AutomatonState *automaton_state, ScannerResult *scanner_result, MutableString *mutable_string, Token *token, bool *token_done) {
    // every automaton_state in the following switch represents one state in the FA of scanner
    switch (*automaton_state) {
        case STATE_EOL_RESOLVED:
        case STATE_DEFAULT: // getting first char of a new token
            // get next character in case of whitespace characters
            if (read_char == '\n' || read_char == '\t' || read_char == ' ') {
                // skip for the next char
            } else if (isalpha(read_char) || read_char == '_') {
                // new token should be an identifier
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_ID;
            } else if (isdigit(read_char)) {
                // new token should be a number
                if (read_char == '0') {
                    *automaton_state = STATE_ZERO;
                } else {
                    *automaton_state = STATE_INT;
                }
                mstr_append(mutable_string, read_char);
            } else if (read_char == '+') {
                *automaton_state = STATE_PLUS;
            } else if (read_char == '-') {
                *automaton_state = STATE_MINUS;
            } else if (read_char == '*') {
                *automaton_state = STATE_MULTIPLY;
            } else if (read_char == '/') {
                *automaton_state = STATE_DIVIDE;
            } else if (read_char == ':') {
                *automaton_state = STATE_COLON;
            } else if (read_char == '=') {
                *automaton_state = STATE_ASSIGN;
            } else if (read_char == '!') {
                *automaton_state = STATE_NOT;
            } else if (read_char == '&') {
                *automaton_state = STATE_AMPERSAND;
            } else if (read_char == '|') {
                *automaton_state = STATE_VERTICAL_BAR;
            } else if (read_char == '(') {
                *automaton_state = STATE_LEFT_BRACKET;
            } else if (read_char == ')') {
                *automaton_state = STATE_RIGHT_BRACKET;
            } else if (read_char == '{') {
                *automaton_state = STATE_CURLY_LEFT_BRACKET;
            } else if (read_char == '}') {
                *automaton_state = STATE_CURLY_RIGHT_BRACKT;
            } else if (read_char == '<') {
                *automaton_state = STATE_LESS_THAN;
            } else if (read_char == '>')  {
                *automaton_state = STATE_GREATER_THAN;
            } else if (read_char == '\"') {
                *automaton_state = STATE_STRING;
            } else if (read_char == ',') {
                *automaton_state = STATE_COMMA;
            } else if (read_char == ';') {
                *automaton_state = STATE_SEMICOLON;
            }

            read_char = EMPTY_CHAR;
            break;

        // already have read at least one char of a new token
        case STATE_ID:
            if (isalpha(read_char) || read_char == '_' || isdigit(read_char)) {
                // new token should be identifier
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_ID;
                check_for_keyword(token, mutable_string);
                check_for_bool_values(token, mutable_string);
                *token_done = true;
            }
            break;

        case STATE_KEYWORD:
            token->type = TOKEN_KEYWORD;
            *token_done = true;
            break;

        case STATE_ZERO:
            break;
        case STATE_BINARY:
            break;
        case STATE_BINARY_NUMBER:
            break;
        case STATE_BINARY_UNDERSCORE:
            break;
        case STATE_OCTAL:
            break;
        case STATE_OCTAL_NUMBER:
            break;
        case STATE_OCTAL_UNDERSCORE:
            break;
        case STATE_HEXADECIMAL:
            break;
        case STATE_HEXADECIMAL_NUMBER:
            break;
        case STATE_HEXADECIMAL_UNDERSCORE:
            break;
        case STATE_INT:
            break;
        case STATE_FLOAT:
            break;
        case STATE_FLOAT_DOT:
            break;
        case STATE_FLOAT_EXP_CHAR:
            break;
        case STATE_FLOAT_EXP_SIGN_CHAR:
            break;
        case STATE_FLOAT_EXPONENT:
            break;
        case STATE_PLUS:
            break;
        case STATE_MINUS:
            break;
        case STATE_MULTIPLY:
            break;
        case STATE_DIVIDE:
            break;
        case STATE_PLUS_ASSIGN:
            break;
        case STATE_MINUS_ASSIGN:
            break;
        case STATE_MULTIPLY_ASSIGN:
            break;
        case STATE_DIVIDE_ASSIGN:
            break;
        case STATE_DEFINE:
            break;
        case STATE_ASSIGN:
            break;
        case STATE_EQUAL_TO:
            break;
        case STATE_TRUE:
            break;

        case STATE_FALSE:
            *token_done = true;
            break;

        case STATE_NOT:
            if (read_char == '=') {
                *automaton_state = STATE_NOT_EQUAL_TO;
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_NOT;
                *token_done = true;
            }
            break;

        case STATE_NOT_EQUAL_TO:
            token->type = TOKEN_NOT_EQUAL_TO;
            *token_done = true;
            break;

        case STATE_AMPERSAND:
            if (read_char == '&') {
                *automaton_state = STATE_AND;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL, "%llu:%llu: '&' is not a valid operator. Did you mean '&&'?", line_num, char_num);
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_AND:
            token->type = TOKEN_AND;
            *token_done = true;
            break;

        case STATE_VERTICAL_BAR:
            if (read_char == '|') {
                *automaton_state = STATE_OR;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL, "%llu:%llu: '|' is not a valid operator. Did you mean '||'?", line_num, char_num);
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_OR:
            token->type = TOKEN_OR;
            *token_done = true;
            break;

        case STATE_LEFT_BRACKET:
            token->type = TOKEN_LEFT_BRACKET;
            *token_done = true;
            break;

        case STATE_RIGHT_BRACKET:
            token->type = TOKEN_RIGHT_BRACKET;
            *token_done = true;
            break;

        case STATE_CURLY_LEFT_BRACKET:
            token->type = TOKEN_CURLY_LEFT_BRACKET;
            *token_done = true;
            break;

        case STATE_CURLY_RIGHT_BRACKT:
            token->type = TOKEN_CURLY_RIGHT_BRACKET;
            *token_done = true;
            break;

        case STATE_LESS_THAN:
            if (read_char == '=') {
                *automaton_state = STATE_LESS_OR_EQUAL;
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_LESS_THAN;
                *token_done = true;
            }
            break;

        case STATE_GREATER_THAN:
            if (read_char == '=') {
                *automaton_state = STATE_GREATER_OR_EQUAL;
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_GREATER_THAN;
                *token_done = true;
            }
            break;

        case STATE_LESS_OR_EQUAL:
            token->type = TOKEN_LESS_OR_EQUAL;
            *token_done = true;
            break;

        case STATE_GREATER_OR_EQUAL:
            token->type = TOKEN_GREATER_OR_EQUAL;
            *token_done = true;
            break;

        case STATE_MULTILINE_COMMENT:
            break;
        case STATE_ASTERISK_IN_MULTILINE_COMMENT:
            break;
        case STATE_ONELINE_COMMENT:
            break;
        case STATE_STRING:
            break;
        case STATE_ESCAPE_CHARACTER_IN_STRING:
            break;
        case STATE_ESCAPE_HEXA_IN_STRING:
            break;
        case STATE_ESCAPE_HEXA_ONE_IN_STRING:
            break;
        case STATE_COMMA:
            break;
        case STATE_COLON:
            break;
        case STATE_SEMICOLON:
            break;
    }
    return read_char;
}

static NextCharResult get_next_char(char *read_char) {
    *read_char = (char) getchar();

    if (ferror(stdin)) {
        return NEXT_CHAR_RESULT_ERROR;
    }
    if (feof(stdin)) {
        return NEXT_CHAR_RESULT_EOF;
    }

    return NEXT_CHAR_RESULT_SUCCESS;
}


static void check_for_keyword(Token *token, MutableString *mutable_string) {
    if (strcmp(mstr_content(mutable_string), "bool") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_BOOL;
    } else if (strcmp(mstr_content(mutable_string), "else") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_ELSE;
    } else if (strcmp(mstr_content(mutable_string), "float64") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_FLOAT64;
    } else if (strcmp(mstr_content(mutable_string), "for") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_FOR;
    } else if (strcmp(mstr_content(mutable_string), "func") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_FUNC;
    } else if (strcmp(mstr_content(mutable_string), "if") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_IF;
    } else if (strcmp(mstr_content(mutable_string), "int") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_INT;
    } else if (strcmp(mstr_content(mutable_string), "package") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_PACKAGE;
    } else if (strcmp(mstr_content(mutable_string), "return") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_RETURN;
    } else if (strcmp(mstr_content(mutable_string), "string") == 0) {
        token->type = TOKEN_KEYWORD;
        token->data.keyword_type = KEYWORD_STRING;
    }
}

static void check_for_bool_values(Token *token, MutableString *mutable_string) {
    if (strcmp(mstr_content(mutable_string), "false") == 0) {
        token->type = TOKEN_BOOL;
        token->data.bool_val = false;
    } else if (strcmp(mstr_content(mutable_string), "true") == 0) {
        token->type = TOKEN_BOOL;
        token->data.bool_val = true;
    }
}

