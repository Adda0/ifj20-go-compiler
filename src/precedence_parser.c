/** @file precedence_parser.c
 *
 * IFJ20 compiler
 *
 * @brief Implements the precedence parser module.
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

// Keep track if we are on the right hand side of the expression for id reductions.
bool right_hand_side = false;

bool reduce_not(PrecedenceStack *stack, PrecedenceNode *start) {
    if (start->rptr->data.data_type != CF_BOOL) {
        type_error("expected bool as operand for negation\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_unary_plus(PrecedenceStack *stack, PrecedenceNode *start) {
    if (start->rptr->rptr->data.data_type != CF_INT && start->rptr->rptr->data.data_type != CF_FLOAT) {
        type_error("expected int or float as operand for unary plus\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=start->rptr->rptr->data.data_type};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_unary_minus(PrecedenceStack *stack, PrecedenceNode *start) {
    if (start->rptr->rptr->data.data_type != CF_INT && start->rptr->rptr->data.data_type != CF_FLOAT) {
        type_error("expected int or float as operand for unary minus\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=start->rptr->rptr->data.data_type};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_multiply(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT)) {
        type_error("expected int or float operands for multiplication\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=start->rptr->data.data_type};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

double dabs(double x) {
    return x > 0 ? x : -x;
}

bool reduce_divide(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT)) {
        type_error("expected int or float operands for division\n");
        return false;
    }
    if (type2 == CF_INT) {
        int64_t divider = start->rptr->rptr->rptr->data.data.num_int_val;
        if (divider == 0) {
            stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_DIVISION_BY_ZERO,
                           "Line %u: division by zero constant\n", start->rptr->rptr->rptr->data.context.line_num);
            return false;
        }

    } else if (type2 == CF_FLOAT) {
        double divider = start->rptr->rptr->rptr->data.data.num_float_val;
        if (dabs(divider) < 1e-7) {
            stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_DIVISION_BY_ZERO,
                           "Line %u: division by zero constant\n", start->rptr->rptr->rptr->data.context.line_num);
            return false;
        }
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=start->rptr->data.data_type};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_plus(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT) &&
            (type1 != CF_STRING || type2 != CF_STRING)) {
        type_error("expected int, float or string operands for addition\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=start->rptr->data.data_type};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_minus(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT)) {
        type_error("expected int or float operands for subtraction\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=start->rptr->data.data_type};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_less_than(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT) &&
        (type1 != CF_STRING || type2 != CF_STRING)) {
        type_error("expected int, float or string operands for <\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_greater_than(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT) &&
        (type1 != CF_STRING || type2 != CF_STRING)) {
        type_error("expected int, float or string operands for >\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_less_or_equal(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT) &&
        (type1 != CF_STRING || type2 != CF_STRING)) {
        type_error("expected int, float or string operands for <=\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_greater_or_equal(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT) &&
        (type1 != CF_STRING || type2 != CF_STRING)) {
        type_error("expected int, float or string operands for >=\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_equal_to(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT) &&
        (type1 != CF_STRING || type2 != CF_STRING)) {
        type_error("expected int, float or string operands for ==\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_not_equal_to(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if ((type1 != CF_INT || type2 != CF_INT) && (type1 != CF_FLOAT || type2 != CF_FLOAT) &&
        (type1 != CF_STRING || type2 != CF_STRING)) {
        type_error("expected int, float or string operands for !=\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_and(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if (type1 != CF_BOOL || type2 != CF_BOOL) {
        type_error("expected bool operands for and\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_or(PrecedenceStack *stack, PrecedenceNode *start) {
    STDataType type1 = start->rptr->data.data_type;
    STDataType type2 = start->rptr->rptr->rptr->data.data_type;
    if (type1 != CF_BOOL || type2 != CF_BOOL) {
        type_error("expected bool operands for or\n");
        return false;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_assign(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_define(PrecedenceStack *stack, PrecedenceNode *start) {
    SymbolTable *table = symtable_stack_top(&symtable_stack)->table;
    symtable_add(table, mstr_content(&start->rptr->data.data.str_val), ST_SYMBOL_VAR);
    PrecedenceNode *current = start;
    while (current->data.type != TOKEN_DEFINE) {
        if (current->data.type == SYMB_NONTERMINAL) {
            symtable_add(table, mstr_content(&current->data.data.str_val), ST_SYMBOL_VAR);
        }
        current = current->rptr;
    }
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_plus_assign(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_minus_assign(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_multiply_assign(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_divide_assign(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_brackets(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=start->rptr->rptr->data.data_type};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_id(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_UNKNOWN, .data=start->rptr->data.data};
    if (right_hand_side) {
        // Variables on RHS must be already defined
        STItem *item = symtable_stack_find_symbol(&symtable_stack, mstr_content(&start->rptr->data.data.str_val));
        if (item == NULL) {
            stderr_message("precedence_parser", ERROR,
                           COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE, "Line %u: "
                            "undefined variable %s\n", start->rptr->data.context.line_num,
                            mstr_content(&start->rptr->data.data.str_val));
            return false;
        }
        new_nonterminal.data_type = item->data.data.var_data.type;
    }
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_int(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_INT, .data=start->rptr->data.data};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_float(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_FLOAT, .data=start->rptr->data.data};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_string(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_STRING, .data=start->rptr->data.data};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool reduce_bool(PrecedenceStack *stack, PrecedenceNode *start) {
    StackSymbol new_nonterminal = {.type=SYMB_NONTERMINAL, .data_type=CF_BOOL, .data=start->rptr->data.data};
    precedence_stack_pop_from(stack, start);
    return precedence_stack_push(stack, new_nonterminal);
}

bool (*semantic_actions[NUMBER_OF_RULES])(PrecedenceStack *stack, PrecedenceNode *start) = {
    reduce_not,
    reduce_unary_plus,
    reduce_unary_minus,
    reduce_multiply,
    reduce_divide,
    reduce_plus,
    reduce_minus,
    reduce_less_than,
    reduce_greater_than,
    reduce_less_or_equal,
    reduce_greater_or_equal,
    reduce_equal_to,
    reduce_not_equal_to,
    reduce_and,
    reduce_or,
    reduce_assign,
    reduce_assign,
    reduce_assign,
    reduce_define,
    reduce_define,
    reduce_define,
    reduce_plus_assign,
    reduce_minus_assign,
    reduce_multiply_assign,
    reduce_divide_assign,
    reduce_brackets,
    reduce_id,
    reduce_int,
    reduce_float,
    reduce_string,
    reduce_bool,
    NULL,
    NULL,
    NULL,
    NULL,
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
        case TOKEN_LEFT_BRACKET:
            return true;
        default:
            return false;
    }
}

bool reduce(PrecedenceStack *stack, PrecedenceNode *start, int *function_level) {
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
            if (rules[i][1] == SYMB_FUNCTION) {
                // Function call reduced, decrease function nesting level
                (*function_level)--;
            }
            return semantic_actions[i](stack, start);
        }
    }
    return false;
}

bool matches_assign_rule(AssignRule assign_rule, int definitions, int assignments, int function_calls,
                         int other_tokens) {
    switch (assign_rule) {
        case VALID_STATEMENT:
            // A statement can only contain one definition or assignment (and the rest is irrelevant)
            // or only a single function call which isn't assigned anywhere.
            if (definitions + assignments == 1 || \
                        (function_calls == 1 && (definitions + assignments + other_tokens) == 0)) {
                return true;
            } else {
                stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,
                               "Line %u: not a valid statement (must be a function call, assignment or definition)\n",
                               token.context.line_num);
                return false;
            }
        case ASSIGN_REQUIRED:
            if (assignments == 1 && definitions == 0) {
                return true;
            } else {
                stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,
                               "Line %u: expected assignment inside expression\n", token.context.line_num);
                return false;
            }
        case DEFINE_REQUIRED:
            if (definitions == 1 && assignments == 0) {
                return true;
            } else {
                stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,
                               "Line %u: expected definition inside expression\n", token.context.line_num);
                return false;
            }
        case PURE_EXPRESSION:
            if (definitions == 0 && assignments == 0) {
                return true;
            } else {
                stderr_message("precedence_parser", ERROR, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL,
                               "Line %u: expected pure expression (no definitions or assignments)\n",
                               token.context.line_num);
                return false;
            }
        default:
            return false;
    }
}

void update_token_counters(int type, int *definitions, int *assignments, int *function_calls, int *other_tokens,
                           int *function_level) {
    switch (type) {
        case TOKEN_DEFINE:
            (*definitions)++;
            right_hand_side = true;
            return;
        case TOKEN_ASSIGN:
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MULTIPLY_ASSIGN:
        case TOKEN_DIVIDE_ASSIGN:
            (*assignments)++;
            right_hand_side = true;
            return;
        case SYMB_FUNCTION:
            // We only care about the number of top-level function calls
            if (*function_level == 0) {
                (*function_calls)++;
            }
            (*function_level)++;
            right_hand_side = true;
            return;
        default:
            // We only care about tokens outside of functions
            if (*function_level == 0) {
                (*other_tokens)++;
            }
            return;
    }
}

StackSymbol copy_token_to_symbol() {
    StackSymbol result;
    result.type = token.type;
    result.data = token.data;
    result.context = token.context;
    return result;
}

int parse_expression(AssignRule assign_rule, bool eol_before_allowed) {
    // Check if there is any expression to read
    if (get_table_index(token.type, eol_before_allowed, token.context.eol_read) == INDEX_END) {
        token_error("expected expression, got %s");
        syntax_error();
    }

    PrecedenceStack stack;
    precedence_stack_init(&stack);
    StackSymbol terminal_symb = {.type=SYMB_END};
    StackSymbol start_symb = {.type=SYMB_BEGIN};
    if (!precedence_stack_push(&stack, terminal_symb)) {
        return COMPILER_RESULT_ERROR_INTERNAL;
    }

    // Keep track of counts of various symbols to be able to check whether assign rule was satisfied
    int definitions = 0;
    int assignments = 0;
    int function_calls = 0;
    int function_level = 0;
    int other_tokens = 0;
    bool eol_allowed = true;
    bool done = false;
    while (!done) {
        StackSymbol current_symbol = copy_token_to_symbol();
        PrecedenceNode *top = precedence_stack_top(&stack);
        PrecedenceNode *to_reduce;
        if (top == NULL) {
            stderr_message("precendece_parser", ERROR, COMPILER_RESULT_ERROR_INTERNAL, "no terminal on stack\n");
            return COMPILER_RESULT_ERROR_INTERNAL;
        }
        int top_index = get_table_index(top->data.type, true, true); // we don't care about EOLs in the stack
        int symbol_index = get_table_index(current_symbol.type, eol_allowed, current_symbol.context.eol_read);
        if (top_index == -1 || symbol_index == -1) {
            syntax_error();
        }
        if (current_symbol.type == TOKEN_ID && symbol_index == INDEX_F) {
            // Found function, push it to the stack as function so that we don't have to
            // check next token again when reading from stack
            current_symbol.type = SYMB_FUNCTION;
        } else if (current_symbol.type == TOKEN_ID && symbol_index == INDEX_I) {
            current_symbol.type = SYMB_ID;
        }
        switch (precedence_table[top_index][symbol_index]) {
            case '=':
                if (!precedence_stack_push(&stack, current_symbol)) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                update_token_counters(current_symbol.type, &definitions, &assignments, &function_calls, &other_tokens,
                                      &function_level);
                check_new_token(EOL_OPTIONAL);
                eol_allowed = eol_allowed_after(prev_token.type);
                break;
            case '<':
                if (!precedence_stack_post_insert(&stack, top, start_symb)) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                if (!precedence_stack_push(&stack, current_symbol)) {
                    return COMPILER_RESULT_ERROR_INTERNAL;
                }
                update_token_counters(current_symbol.type, &definitions, &assignments, &function_calls, &other_tokens,
                                      &function_level);
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
                if (!reduce(&stack, to_reduce, &function_level)) {
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

    if (!matches_assign_rule(assign_rule, definitions, assignments, function_calls, other_tokens)) {
        syntax_error();
    }
    precedence_stack_dispose(&stack);
    syntax_ok();
}
