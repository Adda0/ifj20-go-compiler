/** @file precedence_parser.h
 *
 * IFJ20 compiler
 *
 * @brief Contains declarations of functions and data types for the precedence parser module.
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#ifndef _PRECEDENCE_PARSER_H
#define _PRECEDENCE_PARSER_H 1

#include "scanner.h"

#define NUMBER_OF_OPS 27
#define RULE_LENGTH 8
#define NUMBER_OF_RULES 35

typedef enum assign_rule {
    VALID_STATEMENT, // assign and define is allowed but line can't contain pure expression
                     // such as a + 5 (used for statement expansion)
    ASSIGN_REQUIRED, // assign required
    DEFINE_REQUIRED, // define required
    PURE_EXPRESSION, // define and assign forbidden
} AssignRule;

typedef enum rule_symbol {
    SYMB_NONTERMINAL = TOKEN_SEMICOLON + 1,
    SYMB_END,
    SYMB_BEGIN,
    SYMB_FUNCTION,
    SYMB_ID,
    SYMB_MULTI_NONTERMINAL,
    SYMB_UNDEF,
} RuleSymbol;

typedef enum indices {
    INDEX_NOT,
    INDEX_UNARY_PLUS,
    INDEX_UNARY_MINUS,
    INDEX_MULTIPLY,
    INDEX_DIVIDE,
    INDEX_PLUS,
    INDEX_MINUS,
    INDEX_GREATER_THAN,
    INDEX_LESS_THAN,
    INDEX_GREATER_OR_EQUAL,
    INDEX_LESS_OR_EQUAL,
    INDEX_EQUAL_TO,
    INDEX_NOT_EQUAL_TO,
    INDEX_AND,
    INDEX_OR,
    INDEX_ASSIGN,
    INDEX_DEFINE,
    INDEX_PLUS_ASSIGN,
    INDEX_MINUS_ASSIGN,
    INDEX_MULTIPLY_ASSIGN,
    INDEX_DIVIDE_ASSIGN,
    INDEX_LEFT_BRACKET,
    INDEX_RIGHT_BRACKET,
    INDEX_I,
    INDEX_F,
    INDEX_COMMA,
    INDEX_END,
} TableIndex;

/** @brief Parses an expression starting at current token.
 *
 * @param assign_rule Whether assign and define is allowed in the expression.
 * @param eol_before_allowed Specifies whether EOL was allowed preceeding the current token.
 * @return 0 on successful expression parsing, non-zero otherwise (see compiler.h).
 */
int parse_expression(AssignRule assign_rule, bool eol_before_allowed);

#endif
