#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

const char *atname(ASTNodeType t) {
    switch (t) {
        case AST_ADD:
            return "ADD";
        case AST_SUBTRACT:
            return "SUB";
        case AST_MULTIPLY:
            return "MUL";
        case AST_DIVIDE:
            return "DIV";
        case AST_AR_NEGATE:
            return "NEG";
        case AST_LOG_NOT:
            return "NOT";
        case AST_LOG_AND:
            return "AND";
        case AST_LOG_OR:
            return "OR";
        case AST_LOG_EQ:
            return "EQ";
        case AST_LOG_NEQ:
            return "NEQ";
        case AST_LOG_LT:
            return "LT";
        case AST_LOG_GT:
            return "GT";
        case AST_LOG_LTE:
            return "LTE";
        case AST_LOG_GTE:
            return "GTE";
        case AST_ASSIGN:
            return "ASG";
        case AST_DEFINE:
            return "DEF";
        case AST_FUNC_CALL:
            return "FUN";
        case AST_LIST:
            return "LST";
        case AST_ID:
            return "ID:";
        case AST_CONST_INT:
            return "INT";
        case AST_CONST_FLOAT:
            return "FLO";
        case AST_CONST_STRING:
            return "STR";
        case AST_CONST_BOOL:
            return "BOL";
        default:
            return "???";
    }
}

const char *tname(ASTDataType d) {
    switch (d) {
        case CF_UNKNOWN:
            return "?";
        case CF_UNKNOWN_UNINFERRABLE:
            return "?!";
        case CF_INT:
            return "int";
        case CF_FLOAT:
            return "float";
        case CF_STRING:
            return "string";
        case CF_BOOL:
            return "bool";
        case CF_MULTIPLE:
            return "**";
        case CF_BLACK_HOLE:
            return "_";
        case CF_NIL:
            return "nil";
        default:
            return "???";
    }
}

void print_node_data(ASTNode *node) {
    if (node->dataCount == 0) return;

    switch (node->actionType) {
        case AST_ID:
            printf("%s", node->data[0].symbolTableItemPtr->identifier);
            break;
        case AST_CONST_INT:
            printf("%li", node->data[0].intConstantValue);
            break;
        case AST_CONST_FLOAT:
            printf("%f", node->data[0].floatConstantValue);
            break;
        case AST_CONST_STRING:
            printf("%s", node->data[0].stringConstantValue);
            break;
        case AST_CONST_BOOL:
            printf("%s", (node->data[0].boolConstantValue ? "true" : "false"));
            break;
        default:
            printf("wtf");
            break;
    }
}

void print_ast_int(ASTNode *node, char *sufix, char fromdir) {
    if (node != NULL) {
        char *suf2 = (char *) malloc(strlen(sufix) + 4);
        strcpy(suf2, sufix);
        if (fromdir == 'L') {
            suf2 = strcat(suf2, "  |");
            printf("%s\n", suf2);
        } else
            suf2 = strcat(suf2, "   ");
        print_ast_int(node->right, suf2, 'R');

        printf("%s  +-[%s %s", sufix, tname(node->inheritedDataType), atname(node->actionType));
        print_node_data(node);
        printf("]\n");

        strcpy(suf2, sufix);
        if (fromdir == 'R')
            suf2 = strcat(suf2, "  |");
        else
            suf2 = strcat(suf2, "   ");
        print_ast_int(node->left, suf2, 'L');
        if (fromdir == 'R') printf("%s\n", suf2);
        free(suf2);
    }
}

void ast_print(ASTNode *node) {
    print_ast_int(node, " ", 'X');
}
