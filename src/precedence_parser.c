/** @file precedence_parser.c
 *
 * IFJ20 compiler
 *
 * @brief Contains implementation of precedence parser
 *
 * @author František Nečas (xnecas27), FIT BUT
 */

#include <stdbool.h>
#include <stdio.h>
#include "scanner.h"
#include "parser.h"
#include "precedence_parser.h"
#include "stderr_message.h"
#include "compiler.h"
#include "stacks.h"

const char precedence_table[][NUMBER_OF_OPS] = {
    {'<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // !
    {'<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // + (unary)
    {'<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // - (unary)
    {'<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // *
    {'<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // /
    {'<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // +
    {'<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // -
    {'<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // >
    {'<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // <
    {'<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // >=
    {'<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // <=
    {'<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // ==
    {'<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // !=
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // &&
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '>', '<', '<', '>', '>'}, // ||
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', ' ', '<', '<', '=', '>'}, // =
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', ' ', '<', '<', '=', '>'}, // :=
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', ' ', '<', '<', ' ', '>'}, // +=
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', ' ', '<', '<', ' ', '>'}, // -=
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', ' ', '<', '<', ' ', '>'}, // *=
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', ' ', '<', '<', ' ', '>'}, // /=
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        ' ', ' ', ' ', ' ', ' ', ' ', '<', '=', '<', '<', '=', ' '}, // (
    {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', ' ', ' ', '>', '>'}, // )
    {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
        '>', '>', '>', '>', '>', '>', ' ', '>', ' ', ' ', '>', '>'}, // i
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', '=', ' ', ' ', ' ', ' ', ' '}, // f
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        '=', '=', '=', '=', '=', '=', '<', '=', '<', '<', '=', '>'}, // ,
    {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<',
        '<', '<', '<', '<', '<', '<', '<', ' ', '<', '<', '<', 'o'}, // $
};


int rules[NUMBER_OF_RULES][RULE_LENGTH] = {
    {SYMB_NONTERMINAL, TOKEN_NOT, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, TOKEN_PLUS, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, TOKEN_MINUS, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_MULTIPLY, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_DIVIDE, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_PLUS, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_MINUS, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_LESS_THAN, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_GREATER_THAN, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_LESS_OR_EQUAL, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_GREATER_OR_EQUAL, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_EQUAL_TO, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_NOT_EQUAL_TO, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_AND, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_OR, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_ASSIGN, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_COMMA, SYMB_MULTI_NONTERMINAL, TOKEN_ASSIGN,
        SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_COMMA, SYMB_MULTI_NONTERMINAL, TOKEN_ASSIGN,
        SYMB_NONTERMINAL, TOKEN_COMMA, SYMB_MULTI_NONTERMINAL},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_DEFINE, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_COMMA, SYMB_MULTI_NONTERMINAL, TOKEN_DEFINE,
        SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_COMMA, SYMB_MULTI_NONTERMINAL, TOKEN_DEFINE, SYMB_NONTERMINAL,
        TOKEN_COMMA, SYMB_MULTI_NONTERMINAL},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_PLUS_ASSIGN, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_MINUS_ASSIGN, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_MULTIPLY_ASSIGN, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_DIVIDE_ASSIGN, SYMB_NONTERMINAL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, TOKEN_LEFT_BRACKET, SYMB_NONTERMINAL, TOKEN_RIGHT_BRACKET, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_ID, SYMB_UNDEF},
    {SYMB_NONTERMINAL, TOKEN_INT, SYMB_UNDEF},
    {SYMB_NONTERMINAL, TOKEN_FLOAT, SYMB_UNDEF},
    {SYMB_NONTERMINAL, TOKEN_STRING, SYMB_UNDEF},
    {SYMB_NONTERMINAL, TOKEN_BOOL, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_FUNCTION, TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_FUNCTION, TOKEN_LEFT_BRACKET, SYMB_NONTERMINAL, TOKEN_RIGHT_BRACKET, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_FUNCTION, TOKEN_LEFT_BRACKET, SYMB_NONTERMINAL, TOKEN_COMMA,
        SYMB_MULTI_NONTERMINAL, TOKEN_RIGHT_BRACKET, SYMB_UNDEF},
    {SYMB_NONTERMINAL, SYMB_NONTERMINAL, TOKEN_COMMA, SYMB_MULTI_NONTERMINAL, SYMB_UNDEF},
};

int get_table_index(int type, bool eol_allowed, bool eol_read) {
    if (!eol_allowed && eol_read) {
        return INDEX_END;
    }
    Token tmp;
    switch (type) {
        case TOKEN_NOT: return INDEX_NOT;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            // Check if it is unary or binary
            switch (prev_token.type) {
                case TOKEN_STRING:
                case TOKEN_FLOAT:
                case TOKEN_INT:
                case TOKEN_BOOL:
                case TOKEN_RIGHT_BRACKET:
                    return (token.type == TOKEN_PLUS) ? INDEX_PLUS : INDEX_MINUS;
                default:
                    return (token.type == TOKEN_PLUS) ? INDEX_UNARY_PLUS: INDEX_UNARY_MINUS;
            }
        case TOKEN_MULTIPLY: return INDEX_MULTIPLY;
        case TOKEN_DIVIDE: return INDEX_DIVIDE;
        case TOKEN_GREATER_THAN: return INDEX_GREATER_THAN;
        case TOKEN_LESS_THAN: return INDEX_LESS_THAN;
        case TOKEN_LESS_OR_EQUAL: return INDEX_LESS_OR_EQUAL;
        case TOKEN_GREATER_OR_EQUAL: return INDEX_GREATER_OR_EQUAL;
        case TOKEN_EQUAL_TO: return INDEX_EQUAL_TO;
        case TOKEN_NOT_EQUAL_TO: return INDEX_NOT_EQUAL_TO;
        case TOKEN_AND: return INDEX_AND;
        case TOKEN_OR: return INDEX_OR;
        case TOKEN_ASSIGN: return INDEX_ASSIGN;
        case TOKEN_DEFINE: return INDEX_DEFINE;
        case TOKEN_PLUS_ASSIGN: return INDEX_PLUS_ASSIGN;
        case TOKEN_MINUS_ASSIGN: return INDEX_MINUS_ASSIGN;
        case TOKEN_MULTIPLY_ASSIGN: return INDEX_MULTIPLY_ASSIGN;
        case TOKEN_DIVIDE_ASSIGN: return INDEX_DIVIDE_ASSIGN;
        case TOKEN_LEFT_BRACKET: return INDEX_LEFT_BRACKET;
        case TOKEN_RIGHT_BRACKET: return INDEX_RIGHT_BRACKET;
        case TOKEN_STRING:
        case TOKEN_FLOAT:
        case TOKEN_INT:
        case TOKEN_BOOL:
            return INDEX_I;
        case TOKEN_ID:
            if ((scanner_result = get_token(&tmp, EOL_FORBIDDEN, true)) == SCANNER_RESULT_INTERNAL_ERROR
                || scanner_result == SCANNER_RESULT_INVALID_STATE || scanner_result == SCANNER_RESULT_NUMBER_OVERFLOW) {
                return -1;
            } else if (scanner_result == SCANNER_RESULT_EOF) {
                return INDEX_END;
            } else if (scanner_result == SCANNER_RESULT_EXCESS_EOL) {
                return INDEX_I;
            }
            if (tmp.type == TOKEN_LEFT_BRACKET) {
                return INDEX_F;
            } else {
                return INDEX_I;
            }
        case SYMB_FUNCTION: return INDEX_F;
        case SYMB_ID: return INDEX_I;
        case TOKEN_COMMA: return INDEX_COMMA;
        default: return INDEX_END;
    }
}

bool eol_allowed_after(int type) {
    // EOL is allowed after operators
    switch (type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MULTIPLY_ASSIGN:
        case TOKEN_DIVIDE_ASSIGN:
        case TOKEN_DEFINE:
        case TOKEN_ASSIGN:
        case TOKEN_EQUAL_TO:
        case TOKEN_NOT:
        case TOKEN_NOT_EQUAL_TO:
        case TOKEN_AND:
        case TOKEN_OR:
        case TOKEN_LESS_THAN:
        case TOKEN_LESS_OR_EQUAL:
        case TOKEN_GREATER_THAN:
        case TOKEN_GREATER_OR_EQUAL:
        case TOKEN_COMMA:
            return true;
        default:
            return false;
    }
}

bool reduce(PrecedenceStack *stack, PrecedenceNode *start) {
    for (int i = 0; i < NUMBER_OF_RULES; i++) {
        PrecedenceNode *current = start->rptr;
        bool matches = true;
        for (int j = 1; j < RULE_LENGTH; j++) {
            if (rules[i][j] == SYMB_UNDEF) {
                break;
            }
            if (current == NULL) {
                matches = false;
                break;
            }

            if (rules[i][j] == SYMB_MULTI_NONTERMINAL) {
                while (true) {
                    if (current == NULL) {
                        matches = false;
                        break;
                    }
                    if (current->data.type != SYMB_NONTERMINAL) {
                        matches = false;
                        break;
                    }
                    current = current->rptr;
                    if (current == NULL) {
                        break;
                    }
                    if (current->data.type != TOKEN_COMMA) {
                        break;
                    }
                    current = current->rptr;
                }
            } else {
                if (rules[i][j] != current->data.type) {
                    matches = false;
                    break;
                }
                current = current->rptr;
            }
        }
        if (current == NULL && matches) {
            precedence_stack_pop_from(stack, start);
            Token nonterminal = {.type = rules[i][0]};
            precedence_stack_push(stack, nonterminal);
            return true;
        }
    }
    return false;
}

int parse_expression(AssignRule assign_rule, bool eol_before_allowed) {
    // Check if there is any expression to read
    if (get_table_index(token.type, eol_before_allowed, token.context.eol_read) == INDEX_END) {
        token_error("expected expression, got %s");
        syntax_error();
    }

    PrecedenceStack stack;
    precedence_stack_init(&stack);
    Token terminal_symb = {.type=SYMB_END};
    Token start_symb = {.type=SYMB_BEGIN};
    if (!precedence_stack_push(&stack, terminal_symb)) {
        stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "no memory when pushing onto stack\n");
        return COMPILER_RESULT_ERROR_INTERNAL;
    }

    bool eol_allowed = true;
    bool done = false;
    while (!done) {
        PrecedenceNode *top = precedence_stack_top(&stack);
        PrecedenceNode *to_reduce;
        if (top == NULL) {
            stderr_message("precendece_parser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "no terminal on stack\n");
            return COMPILER_RESULT_ERROR_INTERNAL;
        }
        int top_index = get_table_index(top->data.type, true, true); // we don't care about EOLs in the stack
        int token_index = get_table_index(token.type, eol_allowed, token.context.eol_read);
        if (top_index == -1 || token_index == -1) {
            syntax_error();
        }
        if (token.type == TOKEN_ID && token_index == INDEX_F) {
            // Found function, push it to the stack as function so that we don't have to
            // check next token again when reading from stack
            token.type = SYMB_FUNCTION;
        } else if (token.type == TOKEN_ID && token_index == INDEX_I) {
            token.type = SYMB_ID;
        }
        switch (precedence_table[top_index][token_index]) {
            case '=':
                if (!precedence_stack_push(&stack, token)) {
                    stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                                   "no memory when pushing onto stack\n");
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                check_new_token(EOL_OPTIONAL);
                eol_allowed = eol_allowed_after(prev_token.type);
                break;
            case '<':
                if (!precedence_stack_post_insert(&stack, top, start_symb)) {
                    stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                                   "no memory when pushing onto stack\n");
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                if (!precedence_stack_push(&stack, token)) {
                    stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                                   "no memory when pushing onto stack\n");
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                check_new_token(EOL_OPTIONAL);
                eol_allowed = eol_allowed_after(prev_token.type);
                break;
            case '>':
                to_reduce = precedence_stack_reduce_start(&stack);
                if (to_reduce == NULL) {
                    stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                                   "supposed to reduce but not reduction start found\n");
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                if (!reduce(&stack, to_reduce)) {
                    stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,
                                   "Line %u, col %u: tried to reduce the preceeding expression, no rule found\n",
                                   token.context.line_num, token.context.char_num);
                    syntax_error();
                }
                break;
            case 'o':
                done = true;
                break;
            default:
                stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,
                               "Line %u, col %u: no rule found in the precedence table\n",
                               token.context.line_num, token.context.char_num);
                syntax_error();
        }
    }
    precedence_stack_dispose(&stack);
    syntax_ok();
}