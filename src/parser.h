/** @file parser.h
 *
 * IFJ20 compiler
 *
 * @brief Contains macros and declarations of functions and data types for the recursive descent parser.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#ifndef _PARSER_H
#define _PARSER_H 1

#include "compiler.h"
#include "scanner.h"
#include "stacks.h"
#include "symtable.h"

#define TABLE_SIZE 100

#define type_error(message)                                                                     \
    stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,   \
                   "Line %u" message, token.context.line_num)

#define token_error(message)                                                                    \
    stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,                  \
                   "Line %u, col %u: " message, token.context.line_num,                         \
                   token.context.char_num, convert_token_to_text())

#define eol_error(message)                                                                      \
    stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,                  \
                   "Line %u, col %u: " message, token.context.line_num,                         \
                   token.context.char_num)

#define redefine_error(message)                                                                 \
    stderr_message("parser", ERROR, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE, \
                   "Line %u, col %u: " message, token.context.line_num, token.context.char_num, \
                   mstr_content(&token.data.str_val))

#define recover() do {                                                                          \
    /* Try to recover from the state, find new line and start with <body> from there. */        \
    while (scanner_result != SCANNER_RESULT_EOF) {                                              \
        do {                                                                                    \
            scanner_result = get_token(&token, EOL_OPTIONAL, false);                            \
            if (scanner_result == SCANNER_RESULT_EOF) {                                         \
                return COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL;                               \
            } else if (scanner_result == SCANNER_RESULT_INTERNAL_ERROR) {                       \
                return COMPILER_RESULT_ERROR_INTERNAL;                                          \
            }                                                                                   \
            if (!token.context.eol_read) {                                                      \
                clear_token();                                                                  \
            }                                                                                   \
        } while(!token.context.eol_read);                                                       \
        body();                                                                                 \
    }                                                                                           \
} while(0)

#define check_new_token(eol_rule) do {                                                          \
    prev_token = token;                                                                         \
    if ((scanner_result = get_token(&token, eol_rule, false)) == SCANNER_RESULT_MISSING_EOL) {  \
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

#define semantic_error_redefine() do {                                                          \
    clear_token();                                                                              \
    recover();                                                                                  \
    return COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE;                   \
} while(0)

#define syntax_ok() return COMPILER_RESULT_SUCCESS

extern Token token;
extern Token prev_token;
extern ScannerResult scanner_result;
extern SymtableStack symtable_stack;
extern SymbolTable *function_table;

int body();
void clear_token();
int get_token(Token *token, EolRule eol, bool peek_only);
char *convert_token_to_text();
CompilerResult parser_parse();

#endif
