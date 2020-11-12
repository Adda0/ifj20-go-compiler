/** @file scanner_static.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of static functions of scanner.c.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#ifndef _SCANNER_STATIC_H
#define _SCANNER_STATIC_H 1

#include <stdint.h>
#include <stdbool.h>

#include "mutable_string.h"
#include "scanner.h"
#include "compiler.h"

/**
 * @brief By default, token mutable string length will be set to 16 characters.
 */
#define DEFAULT_TOKEN_LENGTH 16

/**
 * @brief By default, code line length will be set to 128 characters.
 */
#define DEFAULT_CODE_LINE_LENGTH 128

/**
 * @brief When read_char is set ot '\0', we require getting next character from source code.
 */
#define EMPTY_CHAR '\0'

// numeral systems
#define NUMERAL_SYSTEM_BINARY 2
#define NUMERAL_SYSTEM_OCTAL 8
#define NUMERAL_SYSTEM_DECIMAL 10
#define NUMERAL_SYSTEM_HEXADECIMAL 16

/**
 * @brief Data type declaring all the automaton states of the scanner FA.
 */
typedef enum automaton_state {
    STATE_DEFAULT, // nothing was yet assigned to TokenType
    STATE_EOL_RESOLVED, // same as STATE_DEFAULT, but already tested EOL

    STATE_ID,
    STATE_KEYWORD,

    STATE_ZERO, // first char of a number was '0'
    STATE_BINARY,
    STATE_BINARY_NUMBER,
    STATE_BINARY_UNDERSCORE,
    STATE_OCTAL,
    STATE_OCTAL_NUMBER,
    STATE_OCTAL_UNDERSCORE,
    STATE_HEXADECIMAL,
    STATE_HEXADECIMAL_NUMBER,
    STATE_HEXADECIMAL_UNDERSCORE,
    STATE_INT,
    STATE_FLOAT,
    STATE_FLOAT_DOT,
    STATE_FLOAT_EXP_CHAR,
    STATE_FLOAT_EXP_SIGN_CHAR,
    STATE_FLOAT_EXPONENT,

    STATE_PLUS, // +
    STATE_MINUS, // -
    STATE_MULTIPLY, // *
    STATE_DIVIDE, // /

    STATE_PLUS_ASSIGN, // +=
    STATE_MINUS_ASSIGN, // -=
    STATE_MULTIPLY_ASSIGN, // *=
    STATE_DIVIDE_ASSIGN, // /=

    STATE_DEFINE, // :=
    STATE_ASSIGN, // =
    STATE_EQUAL_TO, // ==

    STATE_TRUE,
    STATE_FALSE,

    STATE_NOT, // !
    STATE_NOT_EQUAL_TO, // !=
    STATE_AMPERSAND, // &
    STATE_AND, // &&
    STATE_VERTICAL_BAR, // |
    STATE_OR, // ||

    STATE_LEFT_BRACKET, // (
    STATE_RIGHT_BRACKET, // )
    STATE_CURLY_LEFT_BRACKET, // {
    STATE_CURLY_RIGHT_BRACKT, // }

    STATE_LESS_THAN, // <
    STATE_GREATER_THAN, // >
    STATE_LESS_OR_EQUAL, // <=
    STATE_GREATER_OR_EQUAL, // >=


    STATE_MULTILINE_COMMENT, // /*
    STATE_ASTERISK_IN_MULTILINE_COMMENT, // /* ... *
    STATE_END_OF_MULTILINE_COMMENT, // == TOKEN_DEFAULT, but with eol checks again
    STATE_ONELINE_COMMENT, // //

    STATE_STRING, // "
    STATE_ESCAPE_CHARACTER_IN_STRING, // "...\...
    STATE_ESCAPE_HEXA_IN_STRING, // "...\x...
    STATE_ESCAPE_HEXA_ONE_IN_STRING, // "...\x0...

    STATE_COMMA, // ,
    STATE_COLON, // :
    STATE_SEMICOLON, // ;
} AutomatonState;

/**
 * @brief Get the next character from source code.
 *
 * @param read_char The read character will be stored here.
 * @return NextCharResult Shows whether the read operation ended successfully or not.
 */
static NextCharResult get_next_char(char *read_char);

/**
 * @brief Handles EOL rule for read character.
 *
 * @param eol_rule Current EOL rule given to scanner by parser.
 * @param read_char Character, which has been read.
 * @return EolRuleResult Returns whether EOL rule was successfully applied or not.
 */
static EolRuleResult handle_eol_rule(EolRule eol_rule, char read_char);

/**
 * @brief Resolve the type of the passed characted, that have just been read.
 *
 * @param read_char The character to resolve according to the char and current state.
 * @param line_num Number of the currently read line.
 * @param char_num Number of the currently read char in the line.
 * @param automaton_state The current state of scanner automaton.
 * @param scanner_result The result of the the current token reading.
 * @param mutable_string The string to save the current lexeme.
 * @param token The structure to hold information about the current token.
 * @param token_done Shows whether the token is finished and ready to be returned by scanner.
 * @return char EMPTY_CHAR if the token requires reading another character from the source code, leaves the char unchanged otherwise.
 */
static char resolve_read_char(char read_char, size_t line_num, size_t char_num, AutomatonState *automaton_state,
                              ScannerResult *scanner_result, MutableString *mutable_string, Token *token,
                              bool *token_done);

/**
 * @brief Checks whether found identifier is a keyword or true/false value and sets the appropriate token options.
 *
 * @param token Pointer to the newly created token.
 * @param mutable_string String containing the token read string
 */
static void check_for_keyword(Token *token, MutableString *mutable_string);

/**
 * @brief Checks whether found identifier is a true or false bool value and sets the appropriate token options.
 *
 * @param token Pointer to the newly created token.
 * @param mutable_string String containing the token read string.
 */
static void check_for_bool_values(Token *token, MutableString *mutable_string);

/**
 * @brief Prepares number in MutableString format to be parsed.
 * @details Removes underscores from the number and omits the '0x', '0o', '0b' prefixes from the string.
 * @param mutable_string Mutable string storing number to be parsed.
 * @return char* Number string without underscores prepared to be parsed.
 */
static char *prepare_number_for_parsing(MutableString *mutable_string);

#endif // _SCANNER_STATIC_H
