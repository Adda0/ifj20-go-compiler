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

#define check_new_token(eol_rule) do {                                                          \
    if ((scanner_result = scanner_get_token(&token, eol_rule)) == SCANNER_RESULT_MISSING_EOL || \
            scanner_result == SCANNER_RESULT_EXCESS_EOL) {                                      \
        return COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL;                                       \
    } else if (scanner_result == SCANNER_RESULT_INVALID_STATE ||                                \
            scanner_result == SCANNER_RESULT_NUMBER_OVERFLOW) {                                 \
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
    return COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL;                                           \
} while(0)

#define syntax_ok() return COMPILER_RESULT_SUCCESS



CompilerResult parser_parse();

#endif
