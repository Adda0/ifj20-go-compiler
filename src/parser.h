/** @file parser.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of functions and data types for IFJ20 parser.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#ifndef _PARSER_H
#define _PARSER_H 1

#include "compiler.h"

#define token_error(message)                                                                    \
    stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,                  \
                   "Line %u, col %u: " message, token.context.line_num,                         \
                   token.context.char_num, convert_token_to_text())

#define recover() do {                                                                          \
    /* Try to recover from the state, find new line and start with <body> from there. */        \
    while (scanner_result != SCANNER_RESULT_EOF) {                                              \
        while (!token.context.eol_read) {                                                       \
            scanner_result = scanner_get_token(&token, EOL_OPTIONAL);                           \
            if (scanner_result == SCANNER_RESULT_EOF) {                                         \
                return COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL;                               \
            } else if (scanner_result == SCANNER_RESULT_INTERNAL_ERROR) {                       \
                return COMPILER_RESULT_ERROR_INTERNAL;                                          \
            }                                                                                   \
            if (!token.context.eol_read) {                                                      \
                clear_token();                                                                  \
            }                                                                                   \
        }                                                                                       \
        body();                                                                                 \
        scanner_result = scanner_get_token(&token, EOL_OPTIONAL);                               \
    }                                                                                           \
} while(0)

#define check_new_token(eol_rule) do {                                                          \
    if ((scanner_result = scanner_get_token(&token, eol_rule)) == SCANNER_RESULT_MISSING_EOL) { \
        stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,              \
                       "Line %u, col %u: expected newline\n", token.context.line_num,           \
                       token.context.char_num);                                                 \
        recover();                                                                              \
        return COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL;                                       \
    } else if (scanner_result == SCANNER_RESULT_EXCESS_EOL) {                                   \
        stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,              \
                       "Line %u, col %u: got newline when it was forbidden\n",                  \
                        token.context.line_num, token.context.char_num);                        \
        recover();                                                                              \
        return COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL;                                       \
    } else if (scanner_result == SCANNER_RESULT_INVALID_STATE ||                                \
            scanner_result == SCANNER_RESULT_NUMBER_OVERFLOW) {                                 \
        recover();                                                                              \
        return COMPILER_RESULT_ERROR_LEXICAL;                                                   \
    } else if (scanner_result == SCANNER_RESULT_INTERNAL_ERROR) {                               \
        return COMPILER_RESULT_ERROR_INTERNAL;                                                  \
    }                                                                                           \
} while(0)

#define check_nonterminal(func_call) do {                                                       \
    int result = func_call;                                                                     \
    if (result != COMPILER_RESULT_SUCCESS) {                                                    \
        syntax_error();                                                                         \
    }                                                                                           \
} while(0)

#define syntax_error() do {                                                                     \
    clear_token();                                                                              \
    recover();                                                                                  \
    return COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL;                                           \
} while(0)

#define syntax_ok() return COMPILER_RESULT_SUCCESS



CompilerResult parser_parse();

#endif
