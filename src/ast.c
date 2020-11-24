/** @file ast.c
 *
 * IFJ20 compiler
 *
 * @brief Implements the abstract syntax tree.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#include "ast.h"
#include <stdlib.h>

#define AST_ALLOC_CHECK(ptr) do { if ((ptr) == NULL) { ast_error = AST_ERROR_INTERNAL; return; } } while(0)
#define AST_ALLOC_CHECK_RN(ptr) do { if ((ptr) == NULL) { ast_error = AST_ERROR_INTERNAL; return NULL; } } while(0)

ASTError ast_error = AST_NO_ERROR;

ASTNode *ast_node(ASTNodeType nodeType) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    AST_ALLOC_CHECK_RN(node);

    node->actionType = nodeType;
    return node;
}

ASTNode *ast_node_data(ASTNodeType nodeType, unsigned dataCount) {
    ASTNode *node = calloc(1, sizeof(ASTNode) + dataCount * sizeof(ASTNodeData));
    AST_ALLOC_CHECK_RN(node);

    node->actionType = nodeType;
    node->dataCount = dataCount;
    return node;
}

ASTNode *ast_leaf_single_data(ASTNodeType nodeType, ASTNodeData data) {
    ASTNode *node = ast_node_data(nodeType, 1);
    AST_ALLOC_CHECK_RN(node);

    node->data[0] = data;
    ast_infer_leaf_type(node);

    return node;
}

ASTNode *ast_node_list(unsigned dataCount) {
    return ast_node_data(AST_LIST, dataCount);
}

ASTNode *ast_leaf_id(STSymbol *idSymbolPtr) {
    idSymbolPtr->reference_counter++;
    return ast_leaf_single_data(AST_ID, (ASTNodeData) {.symbolTableItemPtr = idSymbolPtr});
}

ASTNode *ast_node_func_call(STSymbol *funcIdSymbolPtr, ASTNode *paramListNode) {
    ASTNode *idNode = ast_leaf_id(funcIdSymbolPtr);
    ASTNode *callNode = ast_node(AST_FUNC_CALL);

    if (idNode == NULL || callNode == NULL) {
        return NULL;
    }

    callNode->left = idNode;
    callNode->right = paramListNode;
    ast_infer_node_type(callNode);

    return callNode;
}

ASTNode *ast_leaf_consti(int i) {
    return ast_leaf_single_data(AST_CONST_INT, (ASTNodeData) {.intConstantValue = i});
}

ASTNode *ast_leaf_constf(double f) {
    return ast_leaf_single_data(AST_CONST_FLOAT, (ASTNodeData) {.floatConstantValue = f});
}

ASTNode *ast_leaf_consts(const char *s) {
    return ast_leaf_single_data(AST_CONST_STRING, (ASTNodeData) {.stringConstantValue = s});
}

ASTNode *ast_leaf_constb(bool b) {
    return ast_leaf_single_data(AST_CONST_BOOL, (ASTNodeData) {.boolConstantValue = b});
}

ASTNode *ast_get_list_root(ASTNode *innerNode) {
    ASTNode *n = innerNode;

    while (n != NULL) {
        if (n->parent->actionType == AST_LIST) {
            for (unsigned i = 0; i < n->parent->dataCount; i++) {
                if (n->parent->data[i].astPtr == n) {
                    return n->parent;
                }
            }
        }

        n = n->parent;
    }

    return NULL;
}

void ast_add_to_list(ASTNode *astList, ASTNode *node, unsigned dataIndex) {
    if (astList == NULL) {
        ast_error = AST_ERROR_INVALID_ARGUMENT;
        return;
    }

    if (astList->actionType != AST_LIST) {
        ast_error = AST_ERROR_INVALID_TARGET;
        return;
    }

    if (astList->dataCount <= dataIndex) {
        ast_error = AST_ERROR_INVALID_ARGUMENT;
        return;
    }

    node->parent = astList;
    astList->data[dataIndex].astPtr = node;
}

unsigned ast_push_to_list(ASTNode *astList, ASTNode *node) {
    if (astList == NULL) {
        ast_error = AST_ERROR_INVALID_ARGUMENT;
        return (unsigned) -1;
    }

    if (astList->dataPointerIndex == astList->dataCount) {
        ast_error = AST_ERROR_LIST_FULL;
        return (unsigned) -1;
    }

    ast_add_to_list(astList, node, astList->dataPointerIndex);
    return astList->dataPointerIndex++;
}

bool ast_infer_leaf_type(ASTNode *node) {
    // Only ID and CONSTs should be leaf nodes
    if (node->inheritedDataType == CF_UNKNOWN_UNINFERRABLE) {
        return false;
    }

    if (node->inheritedDataType != CF_UNKNOWN) {
        return true;
    }

    if (node->actionType == AST_ID) {
        STSymbol *symb = node->data[0].symbolTableItemPtr;

        if (symb == NULL) {
            ast_error = AST_ERROR_SYMBOL_NOT_ASSIGNED;
            node->inheritedDataType = CF_UNKNOWN_UNINFERRABLE;
            return false;
        }

        if (symb->type == ST_SYMBOL_VAR) {
            node->inheritedDataType = symb->data.var_data.type;
        } else {
            if (!symb->data.func_data.defined) {
                // Function not yet defined: set ID type to UNKNOWN and return true
                node->inheritedDataType = CF_UNKNOWN;
                return true;
            }

            STParam *retType = symb->data.func_data.ret_types;

            if (retType == NULL) {
                node->inheritedDataType = CF_NIL;
            } else if (retType->next == NULL) {
                node->inheritedDataType = retType->type;
            } else {
                node->inheritedDataType = CF_MULTIPLE;
            }
        }

        return true;
    }

    node->inheritedDataType = ast_data_type_for_node_type(node->actionType);
    return node->inheritedDataType != CF_UNKNOWN;
}

bool check_unary_node_children(ASTNode *node) {
    if (node->left == NULL) {
        ast_error = AST_ERROR_UNARY_OP_CHILD_NOT_ASSIGNED;
        return false;
    }

    if (node->left->inheritedDataType == CF_UNKNOWN) {
        if (!ast_infer_node_type(node->left)) {
            node->inheritedDataType = CF_UNKNOWN_UNINFERRABLE;
            return false;
        }
    }

    node->inheritedDataType = node->left->inheritedDataType;
    return true;
}

bool check_binary_node_children(ASTNode *node) {
    if (node->left == NULL || node->right == NULL) {
        ast_error = AST_ERROR_BINARY_OP_CHILDREN_NOT_ASSIGNED;
        return false;
    }

    if (node->left->inheritedDataType == CF_UNKNOWN) {
        if (!ast_infer_node_type(node->left)) {
            node->inheritedDataType = CF_UNKNOWN_UNINFERRABLE;
            return false;
        }
    }

    if (node->right->inheritedDataType == CF_UNKNOWN) {
        if (!ast_infer_node_type(node->right)) {
            node->inheritedDataType = CF_UNKNOWN_UNINFERRABLE;
            return false;
        }
    }

    // If we got to a FUNC_CALL for a func that hasn't been yet defined,
    // the children might've ended up being CF_UNKNOWN.
    // In that case, this is not an error. We can try to infer the type from the second child for now,
    // if both are unknown, this node will be unknown as well.
    if (node->left->inheritedDataType == CF_UNKNOWN) {
        if (node->right->inheritedDataType == CF_UNKNOWN) {
            node->inheritedDataType = CF_UNKNOWN;
            return true;
        } else {
            node->inheritedDataType = node->right->inheritedDataType;
            return true;
        }
    } else if (node->right->inheritedDataType == CF_UNKNOWN) {
        node->inheritedDataType = node->left->inheritedDataType;
        return true;
    }

    if (node->left->inheritedDataType != node->right->inheritedDataType) {
        ast_error = AST_ERROR_BINARY_OP_TYPES_MISMATCH;
        node->inheritedDataType = CF_UNKNOWN_UNINFERRABLE;
        return false;
    }

    node->inheritedDataType = node->left->inheritedDataType;
    return true;
}

#define ast_check_arithmetic(node) \
    if ((node)->inheritedDataType != CF_INT && (node)->inheritedDataType != CF_FLOAT) { \
        (node)->inheritedDataType = CF_UNKNOWN_UNINFERRABLE; \
        ast_error = AST_ERROR_INVALID_CHILDREN_TYPE_FOR_OP; \
        return false; \
    }

#define ast_check_logic(node) \
    if ((node)->inheritedDataType != CF_BOOL) { \
        (node)->inheritedDataType = CF_UNKNOWN_UNINFERRABLE; \
        ast_error = AST_ERROR_INVALID_CHILDREN_TYPE_FOR_OP; \
        return false; \
    }

bool ast_infer_node_type(ASTNode *node) {
    if (node->inheritedDataType == CF_UNKNOWN_UNINFERRABLE) {
        return false;
    }

    if (node->inheritedDataType != CF_UNKNOWN) {
        return true;
    }

    switch (node->actionType) {
        case AST_ADD:
        case AST_SUBTRACT:
        case AST_MULTIPLY:
        case AST_DIVIDE:
            if (!check_binary_node_children(node)) return false;
            ast_check_arithmetic(node);
            return true;
        case AST_AR_NEGATE:
            if (!check_unary_node_children(node)) return false;
            ast_check_arithmetic(node);
            return true;
        case AST_LOG_NOT:
            if (!check_unary_node_children(node)) return false;
            ast_check_logic(node);
            return true;
        case AST_LOG_AND:
        case AST_LOG_OR:
        case AST_LOG_EQ:
        case AST_LOG_LT:
        case AST_LOG_GT:
        case AST_LOG_LTE:
        case AST_LOG_GTE:
            if (!check_binary_node_children(node)) return false;
            ast_check_logic(node);
            return true;
        case AST_ASSIGN:
        case AST_DEFINE:
            // ASSIGN and DEFINE nodes don't represent values, so they don't have a data type.
            node->inheritedDataType = CF_NIL;
            return true;
        case AST_FUNC_CALL:
            // AST_FUNC_CALL node is represented as a binary node with an AST_ID node as the left child
            // and AST_LIST as the right child. Its type can thus be inferred from the corresponding AST_ID
            // node, which will be inferred from the symbol table using ast_infer_leaf_type().
            // The right child is not important in this case, so calling check_unary_node_children() is sufficient.
            return check_unary_node_children(node);
        case AST_LIST:
            if (node->dataCount == 0) {
                node->inheritedDataType = CF_NIL;
            } else if (node->dataCount == 1) {
                ASTNode *innerNode = node->data[0].astPtr;
                if (innerNode == NULL ||
                    (innerNode->inheritedDataType == CF_UNKNOWN && !ast_infer_node_type(innerNode))) {
                    node->inheritedDataType = CF_UNKNOWN_UNINFERRABLE;
                    return false;
                } else {
                    node->inheritedDataType = innerNode->inheritedDataType;
                }
            } else {
                node->inheritedDataType = CF_MULTIPLE;
            }
            return true;
        case AST_ID:
        case AST_CONST_INT:
        case AST_CONST_FLOAT:
        case AST_CONST_STRING:
        case AST_CONST_BOOL:
            return ast_infer_leaf_type(node);
    }
}

ASTDataType ast_data_type_for_node_type(ASTNodeType nodeType) {
    switch (nodeType) {
        case AST_LOG_NOT:
        case AST_LOG_AND:
        case AST_LOG_OR:
        case AST_LOG_EQ:
        case AST_LOG_LT:
        case AST_LOG_GT:
        case AST_LOG_LTE:
        case AST_LOG_GTE:
        case AST_CONST_BOOL:
            return CF_BOOL;
        case AST_CONST_INT:
            return CF_INT;
        case AST_CONST_FLOAT:
            return CF_FLOAT;
        case AST_CONST_STRING:
            return CF_STRING;
        default:
            return CF_UNKNOWN;
    }
}
