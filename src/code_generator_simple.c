/** @file code_generator_simple.c
 *
 * IFJ20 compiler
 *
 * @brief Implements a simple target code generator with no .
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#include <stdio.h>
#include "code_generator.h"
#include "stderr_message.h"

#define TCG_DEBUG 1

#if TCG_DEBUG
#define dbg(msg, ...) printf("# --> "); printf((msg),##__VA_ARGS__); putchar('\n')
#else
#define dbg(msg, ...)
#endif

void generate_function(CFFunction *fun, bool isMain) {
    dbg("Function %s", fun->name);

}

void tcg_generate() {
    if (cf_error) {
        stderr_message("codegen", ERROR, COMPILER_RESULT_ERROR_INTERNAL,
                       "Target code generator called on an erroneous CFG (error code %i).", cf_error);
        return;
    }

    puts(".IFJcode20");

    struct program_structure *prog = get_program();
    CFFuncListNode *n = prog->functionList;

    while (n != NULL) {
        generate_function(&n->fun, &n->fun == prog->mainFunc);
        n = n->next;
    }
}