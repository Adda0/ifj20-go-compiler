/** @file stderr_message.h
 *
 * IFJ20 compiler
 *
 * @brief Contains data types and function declarations for error/warning messages to stderr.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#ifndef _STDERR_MESSAGE_H
#define _STDERR_MESSAGE_H

typedef enum message_type {
    ERROR,
    WARNING,
} MessageType;

/**
 * @brief Write formatted error/warning message to stderr.
 * @details Output format: "<module name>: <message type>: <fmt>".
 * @param module Module name.
 * @param message_type ERROR or WARNING.
 * @param fmt Formatted output message.
 * @param ... Additional arguments if necessary.
 */
void stderr_message(const char *module, MessageType message_type, const char *fmt, ...);

#endif //_STDERR_MESSAGE_H
