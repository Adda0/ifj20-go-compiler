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

CompilerResult compiler_result = COMPILER_RESULT_SUCCESS;

int get_char_internal(int *feofi, int *ferrori) {
    int res = getchar();
    *feofi = feof(stdin);
    *ferrori = ferror(stdin);

    return res;
}

int main() {
    parser_parse();
    return compiler_result;
}
