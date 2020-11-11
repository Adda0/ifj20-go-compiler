/** @file compiler.c
 *
 * IFJ20 compiler
 *
 * @brief Main source file for compiler, implementing main function of the compiler.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#include <stdio.h>
#include "compiler.h"

CompilerResult compiler_result = COMPILER_RESULT_SUCCESS;

int get_char_internal(int *feofi, int *ferrori) {
    int res = getchar();
    *feofi = feof(stdin);
    *ferrori = ferror(stdin);

    return res;
}

int main(int argc, char *argv[]) {


    return compiler_result;
}
