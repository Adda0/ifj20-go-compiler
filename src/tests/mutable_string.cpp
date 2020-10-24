/** @file mutable_string.cpp
 *
 * IFJ20 compiler tests
 *
 * @brief Contains tests for the mutable string data structure.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

#include "gtest/gtest.h"

extern "C" {
#include "mutable_string.h"
}

TEST(MutableString, Init) {
    MutableString str;
    ASSERT_TRUE(mstr_init(&str, 1));
    ASSERT_EXIT((mstr_free(&str), exit(0)), ::testing::ExitedWithCode(0), ".*") << "mstr_free() failed.";
}

TEST(MutableString, InitZeroLength) {
    MutableString str;
    ASSERT_FALSE(mstr_init(&str, 0));
    ASSERT_EXIT((mstr_free(&str), exit(0)), ::testing::ExitedWithCode(0), ".*") << "mstr_free() failed.";
}

TEST(MutableString, InitLong) {
    MutableString str;
    ASSERT_TRUE(mstr_init(&str, 512));
    ASSERT_EXIT((mstr_free(&str), exit(0)), ::testing::ExitedWithCode(0), ".*") << "mstr_free() failed.";
}

TEST(MutableString, InitializedIsEmpty) {
    MutableString str;
    mstr_init(&str, 1);
    ASSERT_STREQ(mstr_content(&str), "");
    ASSERT_EQ(mstr_length(&str), 0);

    mstr_free(&str);
}

TEST(MutableString, LongInitializedIsEmpty) {
    MutableString str;
    mstr_init(&str, 512);
    ASSERT_STREQ(mstr_content(&str), "");
    ASSERT_EQ(mstr_length(&str), 0);

    mstr_free(&str);
}

TEST(MutableString, AppendOne) {
    MutableString str;
    mstr_init(&str, 1);
    ASSERT_TRUE(mstr_append(&str, 'X'));
    ASSERT_STREQ(mstr_content(&str), "X");
    ASSERT_EQ(mstr_length(&str), 1);

    mstr_free(&str);
}

TEST(MutableString, AppendMany) {
    MutableString str;
    mstr_init(&str, 1);

    std::string cmpStr;
    for (int i = 0; i < 64; i++) {
        ASSERT_TRUE(mstr_append(&str, 'X'));
        cmpStr += 'X';
        ASSERT_STREQ(mstr_content(&str), cmpStr.c_str());
    }

    mstr_free(&str);
}

TEST(MutableString, AppendHasNullCharNoExtension) {
    MutableString str;
    mstr_init(&str, 2);
    ASSERT_TRUE(mstr_append(&str, 'X'));
    ASSERT_EQ(mstr_content(&str)[1], '\0');

    mstr_free(&str);
}

TEST(MutableString, AppendHasNullCharExtension) {
    MutableString str;
    mstr_init(&str, 2);
    ASSERT_TRUE(mstr_append(&str, 'X'));
    ASSERT_TRUE(mstr_append(&str, 'Y'));
    ASSERT_EQ(mstr_content(&str)[1], 'Y');
    ASSERT_EQ(mstr_content(&str)[2], '\0');

    mstr_free(&str);
}
