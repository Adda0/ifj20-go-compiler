/** @file stderr_message.c
 *
 * IFJ20 compiler
 *
 * @brief Implements message to stderr informing the user about errors and warnings in the source code.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#include <stdio.h>
#include <stdarg.h>

#include "stderr_message.h"
#include "compiler.h"

void stderr_message(const char *module, MessageType message_type, CompilerResult compiler_result_arg,
                    const char *fmt, ...) {
    set_compiler_result(compiler_result_arg);

    fprintf(stderr, "%s: ", module);
    if (message_type == ERROR) {
        fprintf(stderr, "error: ");
    } else {
        fprintf(stderr, "warning: ");
    }

    va_list arguments;
    va_start(arguments, fmt);
    vfprintf(stderr, fmt, arguments);
    va_end(arguments);
}

void set_compiler_result(CompilerResult compiler_result_arg) {
    if (compiler_result == COMPILER_RESULT_SUCCESS) {
        compiler_result = compiler_result_arg;
    }
}
