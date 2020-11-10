/** @file scanner.c
 *
 * IFJ20 compiler
 *
 * @brief Implements scanner
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <float.h>

#include "scanner.h"
#include "scanner_static.h"
#include "stderr_message.h"


ScannerResult scanner_get_token(Token *token, EolRule eol_rule) {
    // 'static' solves the problem of reading one more char before returning Token
    static char read_char = EMPTY_CHAR; // the last read character from the source code
    static NextCharResult next_char_result = NEXT_CHAR_RESULT_SUCCESS; // result returned when reading read_char
    static size_t line_num = 0; // number of current line
    static size_t char_num = 0; // number of current char in a line

    EolRuleResult eol_rule_result = EOL_RULE_RESULT_SUCCESS;

    token->type = TOKEN_DEFAULT;
    bool token_done = false; // Whether the token is ready to be send to parser. Simulates accept state in FA.

    AutomatonState automaton_state = STATE_DEFAULT;
    ScannerResult scanner_result = SCANNER_RESULT_SUCCESS;
        if (read_char == EMPTY_CHAR) { // get new character from source code
            next_char_result = get_next_char(&read_char);

            if (next_char_result == NEXT_CHAR_RESULT_ERROR) {
                stderr_message("scanner", ERROR, COMPILER_RESULT_ERROR_LEXICAL, "Reading a next character from the source code failed.");
                mstr_free(&mutable_string);
                return SCANNER_RESULT_INTERNAL_ERROR;
            }

            if (next_char_result == NEXT_CHAR_RESULT_EOF) {
                scanner_result = SCANNER_RESULT_EOF;
            }
        }
    return scanner_result;
}

static NextCharResult get_next_char(char *read_char) {
    *read_char = (char) getchar();

    if (ferror(stdin)) {
        return NEXT_CHAR_RESULT_ERROR;
    }
    if (feof(stdin)) {
        return NEXT_CHAR_RESULT_EOF;
    }

    return NEXT_CHAR_RESULT_SUCCESS;
}

