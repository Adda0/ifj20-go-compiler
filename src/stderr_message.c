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
#include <string.h>
#include <stdlib.h>

#include "stderr_message.h"
#include "compiler.h"

void stderr_message(const char *module, MessageType message_type, CompilerResult compiler_result_arg, const char *fmt, ...) {
    set_compiler_result(compiler_result_arg);

    // Allocate string for formatted message as a string containing string fmt, string module and additional characters
    // of maximal length of 14.
    char *format = (char *) malloc(sizeof(char) * (strlen(fmt) + strlen(module) + 14));
    if (format == NULL) {
        fprintf(stderr, "stderr_message: error: Malloc of message to stderr failed.\n");
        set_compiler_result(COMPILER_RESULT_ERROR_INTERNAL);
        return;
    }

    sprintf(format, "%s: %s: %s\n", module, (message_type == ERROR) ? "error" : "warning", fmt);

    va_list arguments;
    va_start(arguments, fmt);
    vfprintf(stderr, format, arguments);
    va_end(arguments);

    free(format);
}

void set_compiler_result(CompilerResult compiler_result_arg) {
    if (compiler_result == COMPILER_RESULT_SUCCESS) {
        compiler_result = compiler_result_arg;
    }
}
