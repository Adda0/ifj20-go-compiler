/** @file scanner.c
 *
 * IFJ20 compiler
 *
 * @brief Implements the scanner module.
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
    static size_t line_num = 1; // number of current line
    static size_t char_num = 0; // number of current char in a line

    EolRuleResult eol_rule_result = EOL_RULE_RESULT_SUCCESS;

    token->type = TOKEN_DEFAULT;
    token->context.eol_read = false;
    token->context.line_num = line_num;
    token->context.char_num = char_num;
    bool token_done = false; // Whether the token is ready to be send to parser. Simulates accept state in FA.

    AutomatonState automaton_state = STATE_DEFAULT;
    ScannerResult scanner_result = SCANNER_RESULT_SUCCESS;

    //initialize token mutable string
    MutableString mutable_string;
    if (!mstr_init(&mutable_string, DEFAULT_TOKEN_LENGTH)) {
        stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "Initialization of mutable string failed.\n");
        return SCANNER_RESULT_INTERNAL_ERROR;
    }

    while (!token_done) {
        if (next_char_result == NEXT_CHAR_RESULT_EOF) {
            mstr_free(&mutable_string);

            // reset static variables to their default values
            read_char = EMPTY_CHAR;
            line_num = 0;
            char_num = 0;
            next_char_result = NEXT_CHAR_RESULT_SUCCESS;

            return SCANNER_RESULT_EOF;
        }

        if (read_char == EMPTY_CHAR) { // get new character from source code
            next_char_result = get_next_char(&read_char);

            if (read_char == '\n') {
                line_num++;
                char_num = 0;
            } else {
                char_num++;
            }

            if (next_char_result == NEXT_CHAR_RESULT_ERROR) {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Reading the next character from the source code failed.\n");
                mstr_free(&mutable_string);
                return SCANNER_RESULT_INTERNAL_ERROR;
            }

            if (next_char_result == NEXT_CHAR_RESULT_EOF) {
                scanner_result = SCANNER_RESULT_EOF;
            }
        }

        if (automaton_state == STATE_DEFAULT ||
            automaton_state == STATE_ONELINE_COMMENT ||
            automaton_state == STATE_END_OF_MULTILINE_COMMENT) {
            // for a first character test EOL rule
            if (read_char != ' ' && read_char != '\t') {
                eol_rule_result = handle_eol_rule(eol_rule, read_char);

                if (read_char == '\n') {
                    token->context.eol_read = true;
                }

                if (eol_rule_result == EOL_RULE_RESULT_EXCESS_EOL) {
                    scanner_result = SCANNER_RESULT_EXCESS_EOL;
                } else if (eol_rule_result == EOL_RULE_RESULT_MISSING_EOL) {
                    scanner_result = SCANNER_RESULT_MISSING_EOL;
                } else if (automaton_state == STATE_ONELINE_COMMENT) {
                    if (read_char == '\n' && eol_rule_result == EOL_RULE_RESULT_SUCCESS) {
                        scanner_result = SCANNER_RESULT_SUCCESS;
                    }
                } else if (automaton_state == STATE_END_OF_MULTILINE_COMMENT &&
                           eol_rule_result == EOL_RULE_RESULT_SUCCESS &&
                           scanner_result == SCANNER_RESULT_MISSING_EOL) {
                    scanner_result = SCANNER_RESULT_SUCCESS;
                }

                if (automaton_state == STATE_END_OF_MULTILINE_COMMENT &&
                    token->context.eol_read &&
                    eol_rule_result == EOL_RULE_RESULT_MISSING_EOL) {
                    scanner_result = SCANNER_RESULT_SUCCESS;
                }

                if (automaton_state != STATE_ONELINE_COMMENT) {
                    automaton_state = STATE_EOL_RESOLVED;
                }
            }
        }

        read_char = resolve_read_char(read_char, line_num, char_num, &automaton_state, &scanner_result, &mutable_string,
                                      token, &token_done);

        if (scanner_result != SCANNER_RESULT_SUCCESS) {
            if (scanner_result != SCANNER_RESULT_EXCESS_EOL && scanner_result != SCANNER_RESULT_MISSING_EOL &&
                scanner_result != SCANNER_RESULT_EOF) {
                if (automaton_state == STATE_STRING_INVALID) {
                    if (token_done) {
                        break;
                    }
                } else {
                    mstr_free(&mutable_string);
                    return scanner_result;
                }
            }
        } else if (token_done) {
            // token has been initialized and the following char belongs to another token ('id=' etc.) / ends this token (whitespace etc.)
            break;
        }
    }

    if (token->type != TOKEN_ID && token->type != TOKEN_STRING) {
        mstr_free(&mutable_string);
    } else {
        token->data.str_val = mutable_string;
    }

    return scanner_result;
}

static char resolve_read_char(char read_char, size_t line_num, size_t char_num, AutomatonState *automaton_state,
                              ScannerResult *scanner_result, MutableString *mutable_string, Token *token,
                              bool *token_done) {
    // every automaton_state in the following switch represents one state in the FA of scanner
    switch (*automaton_state) {
        case STATE_EOL_RESOLVED:
        case STATE_END_OF_MULTILINE_COMMENT:
        case STATE_DEFAULT: // getting first char of a new token
            // get next character in case of whitespace characters
            if (read_char == '\n' || read_char == '\t' || read_char == ' ' || read_char == EOF || read_char == '\0') {
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
            } else if (read_char == '>') {
                *automaton_state = STATE_GREATER_THAN;
            } else if (read_char == '\"') {
                *automaton_state = STATE_STRING;
            } else if (read_char == ',') {
                *automaton_state = STATE_COMMA;
            } else if (read_char == ';') {
                *automaton_state = STATE_SEMICOLON;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: No token can start with '%c'.\n",
                               line_num, char_num, read_char);
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }

            read_char = EMPTY_CHAR;
            token->context.line_num = line_num;
            token->context.char_num = char_num;
            break;

        // already have read at least one character of a new token
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
            if (read_char == 'b' || read_char == 'B') {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_BINARY;
                read_char = EMPTY_CHAR;
            } else if (read_char == 'o' || read_char == 'O') {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_OCTAL;
                read_char = EMPTY_CHAR;
            } else if (read_char >= '0' && read_char <= '7') {
                mstr_append(mutable_string, 'o');
                *automaton_state = STATE_OCTAL;
            } else if (read_char == 'x' || read_char == 'X') {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_HEXADECIMAL;
                read_char = EMPTY_CHAR;
            } else if (read_char == '.') {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_FLOAT_DOT;
                read_char = EMPTY_CHAR;
            } else if (read_char == '_') {
                *automaton_state = STATE_ZERO_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else if (read_char == '8' || read_char == '9') {
                mstr_append(mutable_string, read_char);
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Invalid octal number, expected an octal digit (0 to 7) following '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            } else {
                *automaton_state = STATE_INT;
                token->data.num_int_val = 0;
                mstr_free(mutable_string);
                token->type = TOKEN_INT;
                *token_done = true;
            }
            break;

        case STATE_ZERO_UNDERSCORE:
            if (read_char == '_') {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected an octal digit (0 to 7) following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            } else if (isdigit(read_char)) {
                *automaton_state = STATE_ZERO;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_BINARY:
            if (read_char >= '0' && read_char <= '1') {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
                *automaton_state = STATE_BINARY_NUMBER;
            } else if (read_char == '_') {
                *automaton_state = STATE_BINARY_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Invalid binary number, expected a binary digit following '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_BINARY_NUMBER:
            if (read_char >= '0' && read_char <= '1') {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
            } else if (read_char == '_') {
                *automaton_state = STATE_BINARY_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else {
                char *number_without_underscores = prepare_number_for_parsing(mutable_string);
                if (number_without_underscores == NULL) {
                    *scanner_result = SCANNER_RESULT_INTERNAL_ERROR;
                    return EMPTY_CHAR;
                }

                char *end_ptr = NULL;
                long long num = strtoll(number_without_underscores, &end_ptr, NUMERAL_SYSTEM_BINARY);
                if (*end_ptr != '\0') { // sanity check, shouldn't happen
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Unexpected character '%c' in a binary number.\n",
                                   line_num, char_num, *end_ptr);
                    *scanner_result = SCANNER_RESULT_INVALID_STATE;
                }
                if (errno == ERANGE || num > LLONG_MAX) { // if given number is bigger that possible
                    errno = 0;
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Binary number %s overflows the largest possible value of an integer.\n",
                                   line_num, char_num, mstr_content(mutable_string));
                    *scanner_result = SCANNER_RESULT_NUMBER_OVERFLOW;
                }
                token->data.num_int_val = (int64_t) num;
                mstr_free(mutable_string);
                free(number_without_underscores);
                token->type = TOKEN_INT;
                *token_done = true;
            }
            break;

        case STATE_BINARY_UNDERSCORE:
            if (read_char >= '0' && read_char <= '1') {
                *automaton_state = STATE_BINARY_NUMBER;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a binary digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_OCTAL:
            if (read_char >= '0' && read_char <= '7') {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
                *automaton_state = STATE_OCTAL_NUMBER;
            } else if (read_char == '_') {
                *automaton_state = STATE_OCTAL_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected an octal digit (0 to 7) following '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_OCTAL_NUMBER:
            if (read_char >= '0' && read_char <= '7') {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
            } else if (read_char == '_') {
                *automaton_state = STATE_OCTAL_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else {
                char *number_without_underscores = prepare_number_for_parsing(mutable_string);
                if (number_without_underscores == NULL) {
                    *scanner_result = SCANNER_RESULT_INTERNAL_ERROR;
                    return EMPTY_CHAR;
                }

                char *end_ptr = NULL;
                long long num = strtoll(number_without_underscores, &end_ptr, NUMERAL_SYSTEM_OCTAL);
                if (*end_ptr != '\0') { // sanity check, shouldn't happen
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Unexpected character '%c' in an octal number.\n",
                                   line_num, char_num, *end_ptr);
                    *scanner_result = SCANNER_RESULT_INVALID_STATE;
                }
                if (errno == ERANGE || num > LLONG_MAX) { // if given number is bigger that possible
                    errno = 0;
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Octal number %s overflows the largest possible value of an integer.\n",
                                   line_num, char_num, mstr_content(mutable_string));
                    *scanner_result = SCANNER_RESULT_NUMBER_OVERFLOW;
                }
                token->data.num_int_val = (int64_t) num;
                mstr_free(mutable_string);
                free(number_without_underscores);
                token->type = TOKEN_INT;
                *token_done = true;
            }
            break;

        case STATE_OCTAL_UNDERSCORE:
            if (read_char >= '0' && read_char <= '7') {
                *automaton_state = STATE_OCTAL_NUMBER;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected an octal digit (0 to 7) following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_HEXADECIMAL:
            if (isxdigit(read_char)) {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
                *automaton_state = STATE_HEXADECIMAL_NUMBER;
            } else if (read_char == '_') {
                *automaton_state = STATE_HEXADECIMAL_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a hexadecimal digit following '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_HEXADECIMAL_NUMBER:
            if (isxdigit(read_char)) {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
            } else if (read_char == '_') {
                *automaton_state = STATE_HEXADECIMAL_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else {
                char *number_without_underscores = prepare_number_for_parsing(mutable_string);
                if (number_without_underscores == NULL) {
                    *scanner_result = SCANNER_RESULT_INTERNAL_ERROR;
                    return EMPTY_CHAR;
                }

                char *end_ptr = NULL;
                long long num = strtoll(number_without_underscores, &end_ptr, NUMERAL_SYSTEM_HEXADECIMAL);
                if (*end_ptr != '\0') { // sanity check, shouldn't happen
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Unexpected character '%c' in a hexadecimal digit.\n",
                                   line_num, char_num, *end_ptr);
                    *scanner_result = SCANNER_RESULT_INVALID_STATE;
                }
                if (errno == ERANGE || num > LLONG_MAX) { // if given number is bigger that possible
                    errno = 0;
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Hexadecimal number %s overflows the largest possible value of an integer.\n",
                                   line_num, char_num, mstr_content(mutable_string));
                    *scanner_result = SCANNER_RESULT_NUMBER_OVERFLOW;
                }
                token->data.num_int_val = (int64_t) num;
                mstr_free(mutable_string);
                free(number_without_underscores);
                token->type = TOKEN_INT;
                *token_done = true;
            }
            break;

        case STATE_HEXADECIMAL_UNDERSCORE:
            if (read_char == '_') {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a hexadecimal digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            } else if (isxdigit(read_char)) {
                *automaton_state = STATE_HEXADECIMAL_NUMBER;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a hexadecimal digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_INT:
            if (isdigit(read_char)) {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
            } else if (read_char == 'e' || read_char == 'E') {
                *automaton_state = STATE_FLOAT_EXP_CHAR;
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
            } else if (read_char == '.') {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_FLOAT_DOT;
                read_char = EMPTY_CHAR;
            } else if (read_char == '_') {
                *automaton_state = STATE_INT_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else {
                char *end_ptr = NULL;
                long long num = strtoll(mstr_content(mutable_string), &end_ptr, NUMERAL_SYSTEM_DECIMAL);
                if (*end_ptr != '\0') { // sanity check, shouldn't happen
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Unexpected character '%c' in a decimal number.\n",
                                   line_num, char_num, *end_ptr);
                    *scanner_result = SCANNER_RESULT_INVALID_STATE;
                }
                if (errno == ERANGE || num > LLONG_MAX) { // if given number is bigger that possible
                    errno = 0;
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Number %s overflows the largest possible value of an integer.\n",
                                   line_num, char_num, mstr_content(mutable_string));
                    *scanner_result = SCANNER_RESULT_NUMBER_OVERFLOW;
                }
                token->data.num_int_val = (int64_t) num;
                mstr_free(mutable_string);
                token->type = TOKEN_INT;
                *token_done = true;
            }
            break;

        case STATE_INT_UNDERSCORE:
            if (read_char == '_') {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            } else if (isdigit(read_char)) {
                *automaton_state = STATE_INT;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_FLOAT:
            if (isdigit(read_char)) {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
            } else if (read_char == 'e' || read_char == 'E') {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_FLOAT_EXP_CHAR;
                read_char = EMPTY_CHAR;
            } else if (read_char == '_') {
                *automaton_state = STATE_FLOAT_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else {
                // get the float number from string and set the token float value
                char *end_ptr = NULL;
                double num = strtod(mstr_content(mutable_string), &end_ptr);
                if (*end_ptr != '\0') {
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Unexpected character '%c' in a float.\n",
                                   line_num, char_num, *end_ptr);
                    *scanner_result = SCANNER_RESULT_INVALID_STATE;
                }
                if (errno == ERANGE || num > DBL_MAX) { // if given number is bigger that possible
                    errno = 0;
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Number %s overflows the largest possible value of a float64.\n",
                                   line_num, char_num, mstr_content(mutable_string));
                    *scanner_result = SCANNER_RESULT_NUMBER_OVERFLOW;
                }
                token->data.num_float_val = (double) num;
                mstr_free(mutable_string);
                token->type = TOKEN_FLOAT;
                *token_done = true;
            }
            break;

        case STATE_FLOAT_UNDERSCORE:
            if (read_char == '_') {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            } else if (isdigit(read_char)) {
                *automaton_state = STATE_FLOAT;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_FLOAT_DOT:
            if (isdigit(read_char)) {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_FLOAT;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit following the decimal point in '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_FLOAT_EXP_CHAR:
            if (isdigit(read_char)) {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_FLOAT_EXPONENT;
                read_char = EMPTY_CHAR;
            } else if (read_char == '+' || read_char == '-') {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_FLOAT_EXP_SIGN_CHAR;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit in the exponent part, following the E in '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_FLOAT_EXP_SIGN_CHAR:
            if (isdigit(read_char)) {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_FLOAT_EXPONENT;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit in the exponent part, following the sign in '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_FLOAT_EXPONENT:
            if (isdigit(read_char)) {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;
            } else if (read_char == '_') {
                *automaton_state = STATE_FLOAT_EXPONENT_UNDERSCORE;
                read_char = EMPTY_CHAR;
            } else { // token is done and is some decimal number with exponent
                // get the float number from string and set the token float value
                char *end_ptr = NULL;
                double num = strtod(mstr_content(mutable_string), &end_ptr);
                if (*end_ptr != '\0') { // sanity check, shouldn't happen
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Unexpected character '%c' in a float number.\n",
                                   line_num, char_num, *end_ptr);
                    *scanner_result = SCANNER_RESULT_INVALID_STATE;
                }
                if (errno == ERANGE || num > DBL_MAX) { // if given number is bigger that possible
                    errno = 0;
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Number %s overflows the largest possible value of a float64.\n",
                                   line_num, char_num, mstr_content(mutable_string));
                    *scanner_result = SCANNER_RESULT_NUMBER_OVERFLOW;
                }
                token->data.num_float_val = (double) num;
                mstr_free(mutable_string);
                *automaton_state = STATE_FLOAT;
                token->type = TOKEN_FLOAT;
                *token_done = true;
            }
            break;

        case STATE_FLOAT_EXPONENT_UNDERSCORE:
            if (read_char == '_') {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            } else if (isdigit(read_char)) {
                *automaton_state = STATE_FLOAT_EXPONENT;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected an digit following the underscore in '%s_'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_PLUS:
            if (read_char == '=') {
                *automaton_state = STATE_PLUS_ASSIGN;
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_PLUS;
                *token_done = true;
            }
            break;

        case STATE_MINUS:
            if (read_char == '=') {
                *automaton_state = STATE_MINUS_ASSIGN;
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_MINUS;
                *token_done = true;
            }
            break;

        case STATE_MULTIPLY:
            if (read_char == '=') {
                *automaton_state = STATE_MULTIPLY_ASSIGN;
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_MULTIPLY;
                *token_done = true;
            }
            break;

        case STATE_DIVIDE:
            if (read_char == '/') {
                *automaton_state = STATE_ONELINE_COMMENT;
                read_char = EMPTY_CHAR;
            } else if (read_char == '*') {
                *automaton_state = STATE_MULTILINE_COMMENT;
                read_char = EMPTY_CHAR;
            } else if (read_char == '=') {
                *automaton_state = STATE_DIVIDE_ASSIGN;
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_DIVIDE;
                *token_done = true;
            }
            break;

        case STATE_PLUS_ASSIGN:
            token->type = TOKEN_PLUS_ASSIGN;
            *token_done = true;
            break;

        case STATE_MINUS_ASSIGN:
            token->type = TOKEN_MINUS_ASSIGN;
            *token_done = true;
            break;

        case STATE_MULTIPLY_ASSIGN:
            token->type = TOKEN_MULTIPLY_ASSIGN;
            *token_done = true;
            break;

        case STATE_DIVIDE_ASSIGN:
            token->type = TOKEN_DIVIDE_ASSIGN;
            *token_done = true;
            break;

        case STATE_DEFINE:
            token->type = TOKEN_DEFINE;
            *token_done = true;
            break;

        case STATE_ASSIGN:
            if (read_char == '=') {
                *automaton_state = STATE_EQUAL_TO;
                read_char = EMPTY_CHAR;
            } else {
                token->type = TOKEN_ASSIGN;
                *token_done = true;
            }
            break;

        case STATE_EQUAL_TO:
            token->type = TOKEN_EQUAL_TO;
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
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: '&' is not a valid operator. Did you mean '&&'?\n",
                               line_num, char_num);
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
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: '|' is not a valid operator. Did you mean '||'?\n",
                               line_num, char_num);
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
            // token is '/*' for sure -> ignore all chars til '*/' is read
            if (read_char == '*') {
                *automaton_state = STATE_ASTERISK_IN_MULTILINE_COMMENT;
            } else if (read_char == EOF) {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Block comment hasn't been terminated. End it with '*/'.\n",
                               line_num, char_num);
                *automaton_state = STATE_END_OF_MULTILINE_COMMENT;
            }
            read_char = EMPTY_CHAR;
            break;

        case STATE_ASTERISK_IN_MULTILINE_COMMENT:
            if (read_char == '/') {
                // multiline comment just ended -> back to the default
                *automaton_state = STATE_END_OF_MULTILINE_COMMENT;
            } else {
                // it was only an asterisk in a comment: /* ... * ... -> back to multiline comment
                *automaton_state = STATE_MULTILINE_COMMENT;
            }
            read_char = EMPTY_CHAR;
            break;

        case STATE_ONELINE_COMMENT:
            // token is '//' for sure -> ignore all chars til EOL is read
            if (read_char == '\n') {
                *automaton_state = STATE_EOL_RESOLVED;
            }
            read_char = EMPTY_CHAR;
            break;

        case STATE_STRING:
            if (read_char == '\"') {
                token->data.str_val = *mutable_string;
                token->type = TOKEN_STRING;
                *token_done = true;
            } else if (read_char == '\\') {
                *automaton_state = STATE_ESCAPE_CHARACTER_IN_STRING;
            } else if (read_char == '\n') {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Unexpected newline in a string.\n",
                               line_num, char_num);
                *automaton_state = STATE_STRING_INVALID;
            } else if (read_char == EOF) {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                       "Line %llu, col %llu: String wasn't properly ended, when EOF was read: '%s'. \n",
                       line_num, char_num, mstr_content(mutable_string));
                        *automaton_state = STATE_STRING_INVALID;
            } else if ((int)read_char <= 31) {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                       "Line %llu, col %llu: Unexpected character in a string following '%s'. \n",
                       line_num, char_num, mstr_content(mutable_string));
                       *automaton_state = STATE_STRING_INVALID;
            } else {
                mstr_append(mutable_string, read_char);
            }
            read_char = EMPTY_CHAR;
            break;

        case STATE_ESCAPE_CHARACTER_IN_STRING:
            if (read_char == 'x') {
                *automaton_state = STATE_ESCAPE_HEXA_IN_STRING;
            } else if (read_char == '\"') {
                mstr_append(mutable_string, '\"');
                *automaton_state = STATE_STRING;
            } else if (read_char == 'n') {
                mstr_append(mutable_string, '\n');
                *automaton_state = STATE_STRING;
            } else if (read_char == 't') {
                mstr_append(mutable_string, '\t');
                *automaton_state = STATE_STRING;
            } else if (read_char == '\\') {
                mstr_append(mutable_string, '\\');
                *automaton_state = STATE_STRING;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Invalid string escape sequence '\\%c'.\n",
                               line_num, char_num, read_char);
                *automaton_state = STATE_STRING_INVALID;
            }
            read_char = EMPTY_CHAR;
            break;

        case STATE_STRING_INVALID:
            if (read_char == '\"') {
                token->type = TOKEN_STRING;
                *token_done = true;
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
                read_char = EMPTY_CHAR;
            } else {
                read_char = EMPTY_CHAR;
            }
            break;

        case STATE_ESCAPE_HEXA_IN_STRING:
            if (isdigit(read_char) || (read_char >= 'a' && read_char <= 'f') ||
                (read_char >= 'A' && read_char <= 'F')) {
                mstr_append(mutable_string, read_char);
                *automaton_state = STATE_ESCAPE_HEXA_ONE_IN_STRING;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a hexadecimal digit in the escape sequence following '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *automaton_state = STATE_STRING_INVALID;
            }
            break;

        case STATE_ESCAPE_HEXA_ONE_IN_STRING:
            if (isdigit(read_char) || (read_char >= 'a' && read_char <= 'f') ||
                (read_char >= 'A' && read_char <= 'F')) {
                mstr_append(mutable_string, read_char);
                read_char = EMPTY_CHAR;

                // get the hexadecimal number from a string and replace it with equivalent char
                char *hexa_number_string = calloc(sizeof(char), 5);
                if (hexa_number_string == NULL) {
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                                   "Malloc of string for a hexadecimal escape sequence in a string failed.\n");
                    *scanner_result = SCANNER_RESULT_INTERNAL_ERROR;
                    break;
                }

                char *hexa_string = mstr_content(mutable_string) + mstr_length(mutable_string) - 2;
                strcpy(hexa_number_string, "0x");
                strcpy(hexa_number_string + 2, hexa_string);

                char *end_ptr = NULL;
                int int_val = strtoul(hexa_number_string, &end_ptr, NUMERAL_SYSTEM_HEXADECIMAL);

                if (*end_ptr != '\0') { // sanity check
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Invalid hexadecimal escape sequence '\\%s'.\n",
                                   line_num, char_num, hexa_number_string);
                    *scanner_result = SCANNER_RESULT_INVALID_STATE;
                }
                if (errno == ERANGE || int_val > INT_MAX) { // another sanity check
                    errno = 0;
                    stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                                   "Line %llu, col %llu: Invalid hexadecimal escape sequence '\\%s'.\n",
                                   line_num, char_num, hexa_number_string);
                    *scanner_result = SCANNER_RESULT_NUMBER_OVERFLOW;
                }

                char c = (char) int_val;
                *(mstr_content(mutable_string) + mstr_length(mutable_string) - 2) = c;
                *(mstr_content(mutable_string) + mstr_length(mutable_string) - 1) = '\0';
                mutable_string->used--;

                free(hexa_number_string);
                *automaton_state = STATE_STRING;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Expected a hexadecimal digit in the escape sequence following '%s'.\n",
                               line_num, char_num, mstr_content(mutable_string));
                *automaton_state = STATE_STRING_INVALID;
            }
            break;

        case STATE_COMMA:
            token->type = TOKEN_COMMA;
            *token_done = true;
            break;

        case STATE_COLON:
            if (read_char == '=') {
                *automaton_state = STATE_DEFINE;
                read_char = EMPTY_CHAR;
            } else {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL,
                               "Line %llu, col %llu: Invalid lexeme: ':%c'. Did you mean ':=' to define a new variable?\n",
                               line_num, char_num, read_char);
                *scanner_result = SCANNER_RESULT_INVALID_STATE;
            }
            break;

        case STATE_SEMICOLON:
            token->type = TOKEN_SEMICOLON;
            *token_done = true;
            break;
    }
    return read_char;
}

// This is a getchar() abstraction that is needed to make mocking
// the input in tests possible.
extern int get_char_internal(int *feof, int *ferror);

static NextCharResult get_next_char(char *read_char) {
    int feofi = 0, ferrori = 0;
    *read_char = (char) get_char_internal(&feofi, &ferrori);
    if ((int) *read_char == 13) {
        // when read char is CR, skip it
        *read_char = (char) get_char_internal(&feofi, &ferrori);
    }

    if (ferrori) {
        return NEXT_CHAR_RESULT_ERROR;
    }
    if (feofi) {
        return NEXT_CHAR_RESULT_EOF;
    }

    return NEXT_CHAR_RESULT_SUCCESS;
}

static EolRuleResult handle_eol_rule(EolRule eol_rule, char read_char) {
    switch (eol_rule) {
        case EOL_FORBIDDEN:
            // When EOL is forbidden and scanner reads EOL, no token is send (values pointer
            // token points to are unchanged) and SCANNER_RESULT_EXCESS_EOL is returned.
            if (read_char == '\n') {
                return EOL_RULE_RESULT_EXCESS_EOL;
            }
            break;
        case EOL_REQUIRED:
            if (read_char != '\n') {
                return EOL_RULE_RESULT_MISSING_EOL;
            }
            break;
        case EOL_OPTIONAL:
            if (read_char == '\n') {
                return EOL_RULE_RESULT_OPTIONAL_EOL;
            }
            break;
        default:
            return EOL_RULE_RESULT_SUCCESS;
            break;
    }
    return EOL_RULE_RESULT_SUCCESS;
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

static char *prepare_number_for_parsing(MutableString *mutable_string) {
    char *number_to_parse = (char *) calloc(mstr_length(mutable_string), sizeof(char));
    if (number_to_parse == NULL) {
        stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Malloc of string for a number in a different numeral system failed.\n");
        return NULL;
    }

    size_t index = 0;

    // omit '0x', '0b', '0o' for strtoll cannot recognize binary and octal format of these and omit '_'
    for (size_t i = 2; i < mstr_length(mutable_string); i++) {
        if (mstr_content(mutable_string)[i] != '_') {
            number_to_parse[index] = mstr_content(mutable_string)[i];
            index++;
        }
    }
    return number_to_parse;
}
