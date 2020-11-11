/** @file scanner.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of functions and data types required for implementation of scanner in scanner.c.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#ifndef _SCANNER_H
#define _SCANNER_H 1

#include <stdint.h>
#include <stdbool.h>

#include "mutable_string.h"
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
 * @brief Return value of get_token() function.
 */
typedef enum scanner_result {
    SCANNER_RESULT_SUCCESS,
    SCANNER_RESULT_MISSING_EOL,
    SCANNER_RESULT_EXCESS_EOL,
    SCANNER_RESULT_EOF,
    SCANNER_RESULT_INVALID_STATE, // parsing source code didn't end in an accept state of scanner FA
    SCANNER_RESULT_NUMBER_OVERFLOW, // overflow of integer of double nubmer when parsing the number from string
    SCANNER_RESULT_INTERNAL_ERROR, // internal operation as malloc() failed
} ScannerResult;

/**
 * @brief Data type parser sends to scanner describing required EOL rule for the current token.
 */
typedef enum eol_rule {
    EOL_REQUIRED,
    EOL_FORBIDDEN,
    EOL_OPTIONAL
} EolRule;

/**
 * @brief Data type declaring all the possible token types. Resembles states of FA.
 */
typedef enum token_type {
    TOKEN_DEFAULT, // nothing was yet assigned to TokenType

    TOKEN_ID,
    TOKEN_KEYWORD,

    TOKEN_INT,
    TOKEN_FLOAT,

    TOKEN_PLUS, // +
    TOKEN_MINUS, // -
    TOKEN_MULTIPLY, // *
    TOKEN_DIVIDE, // /

    TOKEN_PLUS_ASSIGN, // +=
    TOKEN_MINUS_ASSIGN, // -=
    TOKEN_MULTIPLY_ASSIGN, // *=
    TOKEN_DIVIDE_ASSIGN, // /=

    TOKEN_DEFINE, // :=
    TOKEN_ASSIGN, // =
    TOKEN_EQUAL_TO, // ==

    TOKEN_BOOL,

    TOKEN_NOT, // !
    TOKEN_NOT_EQUAL_TO, // !=
    TOKEN_AND, // &&
    TOKEN_OR, // ||

    TOKEN_LEFT_BRACKET, // (
    TOKEN_RIGHT_BRACKET, // )
    TOKEN_CURLY_LEFT_BRACKET, // {
    TOKEN_CURLY_RIGHT_BRACKET, // }

    TOKEN_LESS_THAN, // <
    TOKEN_GREATER_THAN, // >
    TOKEN_LESS_OR_EQUAL, // <=
    TOKEN_GREATER_OR_EQUAL, // >=

    TOKEN_STRING, // "

    TOKEN_COMMA, // ,
    TOKEN_SEMICOLON, // ;
} TokenType;

/**
 * @brief Data type declaring all the possible keyword types.
 */
typedef enum keyword_type {
    KEYWORD_BOOL,
    KEYWORD_ELSE,
    KEYWORD_FLOAT64,
    KEYWORD_FOR,
    KEYWORD_FUNC,
    KEYWORD_IF,
    KEYWORD_INT,
    KEYWORD_PACKAGE,
    KEYWORD_RETURN,
    KEYWORD_STRING,
} KeywordType;


/**
 * @brief Data type containing additional data about token – used by parser, filled by scanner.
 */
typedef union token_data {
    int64_t num_int_val; // used for a value of int number
    double num_float_val; // used for a value of float number
    bool bool_val; // used for boolean values
    MutableString str_val; // used for identifiers and strings – structure with string and additional information about the identifier
    KeywordType keyword_type; // used for keywords
} TokenData;

/**
 * @brief Data type containing additional context info about token - used by parser, filled by scanner.
 */
typedef struct token_context {
    size_t line_num;
    size_t char_num;
    bool eol_read;
} TokenContext;

/**
 * @brief Data type containing all the necessary data for token given to parser by scanner.
 */
typedef struct token {
    TokenType type;
    TokenData data;
    TokenContext context;
} Token;

/**
 * @brief Return values of get_next_char() function.
 */
typedef enum next_char_result {
    NEXT_CHAR_RESULT_SUCCESS,
    NEXT_CHAR_RESULT_ERROR,
    NEXT_CHAR_RESULT_EOF,
} NextCharResult;

/**
 * @brief Return values of handle_eol_rule() function.
 */
typedef enum eol_rule_result {
    EOL_RULE_RESULT_SUCCESS,
    EOL_RULE_RESULT_EXCESS_EOL,
    EOL_RULE_RESULT_MISSING_EOL,
    EOL_RULE_RESULT_OPTIONAL_EOL,
} EolRuleResult;

/**
 * @brief Get the next token from source code.
 *
 * @param token Pointer to the newly created token.
 * @param eol_rule Instructs scanner whether EOL is required/forbidden/optional.
 * @return ScannerResult Value shows whether getting a new token was successful or an error occured.
 */
ScannerResult scanner_get_token(Token *token, EolRule eol_rule);

#endif // _SCANNER_H
