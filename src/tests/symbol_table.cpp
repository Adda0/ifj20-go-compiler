/** @file symbol_table.cpp
 *
 * IFJ20 compiler tests
 *
 * @brief Contains tests for the symbol table data structure.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

#include "gtest/gtest.h"


extern "C" {
#include "symtable.h"
#include "stderr_message.h"
#include "tests_common.h"
}

#define ARR_SIZE 100

TEST(SymTable, STHash) {
    std::string key = "a";
    ASSERT_EQ(symtable_hash(key.c_str()), (unsigned) 'a');
}

TEST(SymTable, STInit) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    ASSERT_EQ(table->arr_size, ARR_SIZE);
    for (size_t i = 0; i < ARR_SIZE; i++) {
        ASSERT_TRUE(table->arr[i] == nullptr);
    }
    ASSERT_EQ(table->size, 0);

    free(table);
}

TEST(SymTable, STAdd1) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    STItem *item = symtable_add(table, "a", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "a");
    ASSERT_STREQ(item->data.identifier, "a");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);

    item = table->arr[97];
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "a");
    ASSERT_STREQ(item->data.identifier, "a");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_EQ(item->next, nullptr);

    symtable_free(table);
}

TEST(SymTable, STAdd2) {
    SymbolTable *table = symtable_init(1);

    STItem *item = symtable_add(table, "a", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "a");
    ASSERT_STREQ(item->data.identifier, "a");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    item = table->arr[0];
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "a");
    ASSERT_STREQ(item->data.identifier, "a");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_EQ(item->next, nullptr);

    item = symtable_add(table, "ab", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "ab");
    ASSERT_STREQ(item->data.identifier, "ab");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    item = table->arr[0];
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "ab");
    ASSERT_STREQ(item->data.identifier, "ab");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_NE(item->next, nullptr);

    item = symtable_add(table, "abc", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "abc");
    ASSERT_STREQ(item->data.identifier, "abc");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    item = table->arr[0];
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "abc");
    ASSERT_STREQ(item->data.identifier, "abc");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_NE(item->next, nullptr);

    item = symtable_add(table, "abcd", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "abcd");
    ASSERT_STREQ(item->data.identifier, "abcd");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    item = table->arr[0];
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "abcd");
    ASSERT_STREQ(item->data.identifier, "abcd");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_NE(item->next, nullptr);

    item = symtable_add(table, "abcde", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "abcde");
    ASSERT_STREQ(item->data.identifier, "abcde");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    item = table->arr[0];
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "abcde");
    ASSERT_STREQ(item->data.identifier, "abcde");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_NE(item->next, nullptr);

    item = symtable_add(table, "abcdef", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "abcdef");
    ASSERT_STREQ(item->data.identifier, "abcdef");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    item = table->arr[0];
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "abcdef");
    ASSERT_STREQ(item->data.identifier, "abcdef");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_NE(item->next, nullptr);

    symtable_free(table);
}

TEST(SymTable, STAdd3) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    symtable_add(table, "a", ST_SYMBOL_VAR);
    ASSERT_EQ(symtable_add(table, "a", ST_SYMBOL_FUNC), nullptr);
    ASSERT_EQ(compiler_result, COMPILER_RESULT_ERROR_INTERNAL);

    symtable_free(table);
}

TEST(SymTable, STFind1) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    symtable_add(table, "a", ST_SYMBOL_VAR);

    STItem *item = symtable_find(table, "a");
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "a");
    ASSERT_STREQ(item->data.identifier, "a");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_EQ(item->next, nullptr);

    symtable_free(table);
}

TEST(SymTable, STFind2) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    symtable_add(table, "a", ST_SYMBOL_VAR);

    STItem *item = symtable_find(table, "b");
    ASSERT_TRUE(item == nullptr);
    symtable_free(table);
}

TEST(SymTable, STFind3) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    STItem *item = symtable_find(table, "a");
    ASSERT_TRUE(item == nullptr);
    symtable_free(table);
}

TEST(SymTable, STFree1) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    symtable_add(table, "a", ST_SYMBOL_VAR);
    symtable_add(table, "foo", ST_SYMBOL_FUNC);
    symtable_add(table, "bar", ST_SYMBOL_VAR);

    symtable_free(table);
}

TEST(SymTable, STFreeEmptyTable) {
    SymbolTable *table = symtable_init(ARR_SIZE);

    symtable_free(table);
}

TEST(SymTable, STComplex) {
    SymbolTable *table = symtable_init(ARR_SIZE);

    STItem *item = symtable_add(table, "a", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "a");
    ASSERT_STREQ(item->data.identifier, "a");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_EQ(table->size, 1);

    item = symtable_add(table, "b", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "b");
    ASSERT_STREQ(item->data.identifier, "b");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_EQ(table->size, 2);

    item = symtable_add(table, "c", ST_SYMBOL_VAR);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "c");
    ASSERT_STREQ(item->data.identifier, "c");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_EQ(table->size, 3);

    item = symtable_add(table, "foo", ST_SYMBOL_FUNC);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "foo");
    ASSERT_STREQ(item->data.identifier, "foo");
    ASSERT_EQ(item->data.type, ST_SYMBOL_FUNC);
    ASSERT_EQ(table->size, 4);

    item = symtable_add(table, "bar", ST_SYMBOL_FUNC);
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "bar");
    ASSERT_STREQ(item->data.identifier, "bar");
    ASSERT_EQ(item->data.type, ST_SYMBOL_FUNC);
    ASSERT_EQ(table->size, 5);


    item = symtable_find(table, "a");
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "a");
    ASSERT_STREQ(item->data.identifier, "a");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_EQ(table->size, 5);

    item = symtable_find(table, "b");
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "b");
    ASSERT_STREQ(item->data.identifier, "b");
    ASSERT_EQ(item->data.type, ST_SYMBOL_VAR);
    ASSERT_EQ(table->size, 5);

    item = symtable_find(table, "foo");
    ASSERT_TRUE(item != nullptr);
    ASSERT_STREQ(item->key, "foo");
    ASSERT_STREQ(item->data.identifier, "foo");
    ASSERT_EQ(item->data.type, ST_SYMBOL_FUNC);
    ASSERT_EQ(table->size, 5);


    item->data.data.func_data.defined = true;
    symtable_add_param(item, "gre", CF_INT);
    ASSERT_EQ(item->data.data.func_data.params->type, CF_INT);
    ASSERT_STREQ(item->data.data.func_data.params->id, "gre");
    symtable_add_ret_type(item, "fre", CF_BOOL);
    ASSERT_EQ(item->data.data.func_data.ret_types->type, CF_BOOL);
    ASSERT_STREQ(item->data.data.func_data.ret_types->id, "fre");

    symtable_free(table);
}

TEST(SymTable, STAddParam1) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    STItem *item = symtable_add(table, "a", ST_SYMBOL_FUNC);

    symtable_add_param(item, "param1", CF_INT);

    item = symtable_find(table, "a");
    ASSERT_EQ(item->data.data.func_data.params->type, CF_INT);
    ASSERT_STREQ(item->data.data.func_data.params->id, "param1");
    symtable_free(table);
}

TEST(SymTable, STAddParam2) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    STItem *item = symtable_add(table, "a", ST_SYMBOL_FUNC);

    symtable_add_param(item, "param1", CF_INT);
    symtable_add_param(item, "param2", CF_BOOL);
    symtable_add_param(item, "param3", CF_UNKNOWN);

    item = symtable_find(table, "a");
    ASSERT_EQ(item->data.data.func_data.params->type, CF_INT);
    ASSERT_STREQ(item->data.data.func_data.params->id, "param1");
    ASSERT_EQ(item->data.data.func_data.params->next->type, CF_BOOL);
    ASSERT_STREQ(item->data.data.func_data.params->next->id, "param2");
    ASSERT_EQ(item->data.data.func_data.params->next->next->type, CF_UNKNOWN);
    ASSERT_STREQ(item->data.data.func_data.params->next->next->id, "param3");

    symtable_free(table);
}

TEST(SymTable, STAddRetType1) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    STItem *item = symtable_add(table, "a", ST_SYMBOL_FUNC);

    symtable_add_ret_type(item, "ret_type1", CF_INT);

    item = symtable_find(table, "a");
    ASSERT_EQ(item->data.data.func_data.ret_types->type, CF_INT);
    ASSERT_STREQ(item->data.data.func_data.ret_types->id, "ret_type1");

    symtable_free(table);
}

TEST(SymTable, STAddRetType2) {
    SymbolTable *table = symtable_init(ARR_SIZE);
    STItem *item = symtable_add(table, "a", ST_SYMBOL_FUNC);

    symtable_add_ret_type(item, "ret_type1", CF_INT);
    symtable_add_ret_type(item, "ret_type2", CF_BOOL);
    symtable_add_ret_type(item, "ret_type3", CF_UNKNOWN);

    item = symtable_find(table, "a");
    ASSERT_EQ(item->data.data.func_data.ret_types->type, CF_INT);
    ASSERT_STREQ(item->data.data.func_data.ret_types->id, "ret_type1");
    ASSERT_EQ(item->data.data.func_data.ret_types->next->type, CF_BOOL);
    ASSERT_STREQ(item->data.data.func_data.ret_types->next->id, "ret_type2");
    ASSERT_EQ(item->data.data.func_data.ret_types->next->next->type, CF_UNKNOWN);
    ASSERT_STREQ(item->data.data.func_data.ret_types->next->next->id, "ret_type3");

    symtable_free(table);
}
