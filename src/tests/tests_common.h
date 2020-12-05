/** @file test_common.h
 *
 * IFJ20 compiler tests
 *
 * @brief Contains helper code to be included in scanner/parser tests.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#ifndef _TESTS_COMMON_H
#define _TESTS_COMMON_H 1

#include "stderr_message.h"
#include <stdarg.h>

CompilerResult compiler_result = COMPILER_RESULT_SUCCESS;

const std::map<CompilerResult, std::string> resultNames = {
        {COMPILER_RESULT_SUCCESS,                                           "Success (0)"},
        {COMPILER_RESULT_ERROR_LEXICAL,                                     "Lexical error (1)"},
        {COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,                         "Syntax error, incl. wrong EOLs (2)"},
        {COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE, "Semantic error: Undefined or redefined symbol (3)"},
        {COMPILER_RESULT_ERROR_WRONG_TYPE_OF_NEW_VARIABLE,                  "Semantic error: Uninferrable variable type (4)"},
        {COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION,          "Semantic error: Type mismatch in expression (5)"},
        {COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE,             "Semantic error: Invalid function call parameters or function return values (6)"},
        {COMPILER_RESULT_ERROR_SEMANTIC_GENERAL,                            "Semantic error: Other (7)"},
        {COMPILER_RESULT_ERROR_DIVISION_BY_ZERO,                            "Division by zero (9)"},
        {COMPILER_RESULT_ERROR_INTERNAL,                                    "Internal error (99)"}
};

int get_char_internal(int *feof, int *ferror) {
    int c = std::cin.get();

    if (c == EOF) {
        *feof = 1;
#if VERBOSE > 2
        std::cout << "[READ EOF]\n";
#endif
        return EOF;
    }

#if VERBOSE > 2
    std::cout << "[READ] " << (char) c << '\n';
    std::cout.flush();
#endif

    return c;
}

void set_compiler_result(CompilerResult compiler_result_arg) {
#if VERBOSE > 1
    std::cout << "[TEST] Set compiler result request. Current val: " << resultNames.find(compiler_result)->second
        << "; Requested val: " << resultNames.find(compiler_result_arg)->second << "\n";
#endif

    if (compiler_result == COMPILER_RESULT_SUCCESS) {
#if VERBOSE > 1
        std::cout << "[TEST] Compiler result changed!\n";
#endif
        compiler_result = compiler_result_arg;
    }
}

void stderr_message(const char *module, MessageType message_type, CompilerResult compiler_result_arg,
                    const char *fmt, ...) {
    set_compiler_result(compiler_result_arg);

    if (message_type == ERROR) {
        std::cout << "[OUT ERROR]";
    } else {
        std::cout << "[OUT WARNING]";
    }

    std::cout << "[" << module << "] ";

    va_list arguments;
    va_start(arguments, fmt);
    std::vprintf(fmt, arguments);
    va_end(arguments);

    std::cout << '\n';
}
#endif