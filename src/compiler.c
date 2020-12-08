/** @file compiler.c
 *
 * IFJ20 compiler
 *
 * @brief Main source file for the compiler.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#include <stdio.h>
#include "compiler.h"
#include "parser.h"
#include "optimiser.h"
#include "control_flow.h"
#include "code_generator.h"

CompilerResult compiler_result = COMPILER_RESULT_SUCCESS;

int get_char_internal(int *feofi, int *ferrori) {
    int res = getchar();
    *feofi = feof(stdin);
    *ferrori = ferror(stdin);

    return res;
}

int main() {
    parser_parse();
    if (compiler_result == COMPILER_RESULT_SUCCESS) {
        optimiser_optimise();
    }
    if (compiler_result == COMPILER_RESULT_SUCCESS) {
        tcg_generate();
    }

    cf_clean_all();
    return compiler_result;
}
