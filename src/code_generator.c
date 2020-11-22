/** @file code_generator.c
 *
 * IFJ20 compiler
 *
 * @brief Contains definitions for the target code generator.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#include <stdio.h>
#include "code_generator.h"

void print_ast(ASTNode *root) {
    if (root != NULL) {
        print_ast(root->left);
        print_ast(root->right);

        switch (root->actionType) {
            case AST_ADD:
                printf("+");
                break;
            case AST_SUBTRACT:
                printf("-");
                break;
            case AST_MULTIPLY:
                printf("*");
                break;
            case AST_DIVIDE:
                printf("/");
                break;
            case AST_AR_NEGATE:
                printf("-");
                break;
            case AST_LOG_NOT:
                printf("!");
                break;
            case AST_LOG_AND:
                printf("&&");
                break;
            case AST_LOG_OR:
                printf("||");
                break;
            case AST_LOG_EQ:
                printf("==");
                break;
            case AST_LOG_LT:
                printf("<");
                break;
            case AST_LOG_GT:
                printf(">");
                break;
            case AST_LOG_LTE:
                printf("<=");
                break;
            case AST_LOG_GTE:
                printf(">=");
                break;
            case AST_ASSIGN:
                printf("ASSIGN");
                break;
            case AST_DEFINE:
                printf("DEFINE");
                break;
            case AST_FUNC_CALL:
                printf("CALL");
                break;
        }

        if (root->dataCount > 1) {
            printf("[[");
        }

        for (unsigned i = 0; i < root->dataCount; i++) {
            ASTNodeData d = root->data[i];
            switch (root->actionType) {
                case AST_LIST:
                    printf("-- AST list member #%u:\n", i);
                    print_ast(d.astPtr);
                    break;
                case AST_ID:
                    printf("ID?");
                    break;
                case AST_CONST_INT:
                    printf("%li", d.intConstantValue);
                    break;
                case AST_CONST_FLOAT:
                    printf("%f", d.floatConstantValue);
                    break;
                case AST_CONST_STRING:
                    printf("'%s'", d.stringConstantValue);
                    break;
                case AST_CONST_BOOL:
                    printf(d.boolConstantValue ? "true" : "false");
                    break;
            }

            if (root->dataCount > 1 && i < root->dataCount - 1) {
                printf("; ");
            }
        }

        if (root->dataCount > 1) {
            printf("]]");
        }

        putchar(' ');
    }
}

void print_stat_rec(CFStatement *stat) {
    if (stat == NULL) {
        printf("-- Empty statement\n");
        return;
    }

    switch (stat->statementType) {
        case CF_BASIC:
            printf("> ");
            print_ast(stat->data.bodyAst);
            putchar('\n');
            break;
        case CF_IF:
            printf("-- IF (expr follows)\n");
            print_ast(stat->data.ifData->conditionalAst);
            printf("\n-- THEN\n");
            print_stat_rec(stat->data.ifData->thenStatement);
            printf("-- ELSE\n");
            print_stat_rec(stat->data.ifData->elseStatement);
            printf("-- END IF\n");
            break;
        case CF_FOR:
            printf("-- FOR\n-- Definition expr:\n");
            print_ast(stat->data.forData->definitionAst);
            printf("\n-- Conditional expr:\n");
            print_ast(stat->data.forData->conditionalAst);
            printf("\n-- Afterthought expr:\n");
            print_ast(stat->data.forData->afterthoughtAst);
            printf("-- BODY\n");
            print_stat_rec(stat->data.forData->bodyStatement);
            printf("-- END FOR\n");
            break;
        case CF_RETURN:
            printf("-- RETURN (expr follows)\n");
            print_ast(stat->data.bodyAst);
            putchar('\n');
            break;
    }

    if (stat->followingStatement != NULL) {
        print_stat_rec(stat->followingStatement);
    }
}

void print_arg_list(CFVarListNode *first) {
    while (first != NULL) {
        printf("   - #%u: Name: '%s', Type: '%i'\n", first->variable.position,
               first->variable.name, first->variable.dataType);
        first = first->next;
    }
}

void print_fun(CFFunction *fun) {
    printf("-- Arguments:\n");
    print_arg_list(fun->arguments);
    printf("-- Return values:\n");
    print_arg_list(fun->returnValues);
    printf("-- Control flow:\n");
    print_stat_rec(fun->rootStatement);
    putchar('\n');
}

void tcg_generate() {
    struct program_structure *prog = get_program();
    CFFuncListNode *n = prog->functionList;
    printf("-- Start --\n");
    while (n != NULL) {
        printf("-- Function '%s' --\n", n->fun.name);
        print_fun(&n->fun);
        putchar('\n');
        n = n->next;
    }
    printf("-- End --");
}