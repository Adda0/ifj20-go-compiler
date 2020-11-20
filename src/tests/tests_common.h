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
    std::cout << "[TEST] Set compiler result request. Current val: " << compiler_result << "; Requested val: " << compiler_result_arg << "\n";
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