/** @file compiler.h
 *
 * IFJ20 compiler
 *
 * @brief Contains general declarations and data types for the main function
 * of the entire compiler in compiler.c.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#ifndef _COMPILER_H
#define _COMPILER_H 1

/**
 * @brief Return codes returned by the compiler.
 */
typedef enum compiler_result {
    COMPILER_RESULT_SUCCESS = 0,
    COMPILER_RESULT_ERROR_LEXICAL = 1,
    COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL = 2,
    COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE = 3,
    COMPILER_RESULT_ERROR_WRONG_TYPE_OF_NEW_VARIABLE = 4,
    COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION = 5,
    COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE = 6,
    COMPILER_RESULT_ERROR_SEMANTIC_GENERAL = 7,
    COMPILER_RESULT_ERROR_DIVISION_BY_ZERO = 9,
    COMPILER_RESULT_ERROR_INTERNAL = 99,
} CompilerResult;

extern CompilerResult compiler_result;

#endif // _COMPILER_H
