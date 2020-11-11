/** @file stderr_message.h
 *
 * IFJ20 compiler
 *
 * @brief Contains data types and function declarations for error/warning messages to stderr.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#ifndef _STDERR_MESSAGE_H
#define _STDERR_MESSAGE_H 1

#include "compiler.h"

typedef enum message_type {
    ERROR,
    WARNING,
} MessageType;

/**
 * @brief Write formatted error/warning message to stderr.
 * @details Output format: "<module name>: <message type>: <fmt>".
 * @param module Module name.
 * @param message_type ERROR or WARNING.
 * @param compiler_result_arg Compiler result to be set and returned when the compiler ends.
 * @param fmt Formatted output message.
 * @param ... Additional arguments if necessary.
 */
void stderr_message(const char *module, MessageType message_type, CompilerResult compiler_result_arg, const char *fmt, ...);

/**
 * @brief Set the compiler result value.
 * @details If result_value is already set to anything other than COMPILER_RESULT_SUCCESS, the function does nothing.
 * @param compiler_result_arg Result value to set the compiler_result to.
 */
void set_compiler_result(CompilerResult compiler_result_arg);

#endif //_STDERR_MESSAGE_H
