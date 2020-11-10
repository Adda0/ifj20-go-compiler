/** @file scanner.cpp
 *
 * IFJ20 compiler tests
 *
 * @brief Contains tests for the scanner module.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */

// Verbosity levels:
// 0 – no debug info
// 1 – info about processed tokens
// 2 – all info excluding character reads and buffer clearing
// 3 – all info
#define VERBOSE 3

#include <iostream>
#include <list>
#include <array>
#include "gtest/gtest.h"

extern "C" {
#include "scanner.h"
#include "mutable_string.h"

CompilerResult compiler_result = COMPILER_RESULT_SUCCESS;

int get_char_internal(int *feof, int *ferror) {
    int c = std::cin.get();

    if (c == EOF) {
        *feof = 1;
#if VERBOSE > 2
        std::cout << "[READ EOF]\n";
#endif
        return EOF;
    }

#if VERBOSE > 2
    std::cout << "[READ] " << (char) c << '\n';
    std::cout.flush();
#endif

    return c;
}
}

union ExpData {
    int intVal;
    double doubleVal;
    bool boolVal;
    const char *strVal;
    KeywordType kw;
};

struct ExpectedToken {
    TokenType type;
    ExpData data;
    EolRule nextEolRule;
    ScannerResult expectedResult;

    ExpectedToken(TokenType expType) {
        type = expType;
        data = {.intVal = 0};
        nextEolRule = EOL_OPTIONAL;
        expectedResult = SCANNER_RESULT_SUCCESS;
    }

    ExpectedToken(TokenType expType, ExpData expData) {
        type = expType;
        data = expData;
        nextEolRule = EOL_OPTIONAL;
        expectedResult = SCANNER_RESULT_SUCCESS;
    }

    ExpectedToken(TokenType expType, EolRule expNextEolRule) {
        type = expType;
        data = {.intVal = 0};
        nextEolRule = expNextEolRule;
        expectedResult = SCANNER_RESULT_SUCCESS;
    }

    ExpectedToken(TokenType expType, ExpData expData, EolRule expNextEolRule) {
        type = expType;
        data = expData;
        nextEolRule = expNextEolRule;
        expectedResult = SCANNER_RESULT_SUCCESS;
    }
};

const std::array<std::string, 7> stateNames =
        {"Success",
         "MissingEol",
         "ExcessEol",
         "EOF",
         "InvalidState",
         "NumberOverflow",
         "InternalError"};

const std::array<std::string, 32> tokenTypeNames = {"Default", "Identifier", "Keyword", "IntVal", "FloatVal",
                                                    "StringVal", "BoolVal", "+", "-", "*", "/", "+=", "-=", "*=", "/=",
                                                    ":=", "=", "==", "!", "!=", "&&", "||", "(", ")", "{", "}", "<",
                                                    ">", "<=", ">=", ",", ";"};

class ScannerTest : public ::testing::Test {
protected:
    std::streambuf *cinBackup;
    std::stringbuf *buffer;

    void SetUp() override {
#if VERBOSE > 1
        std::cout << "[TEST SetUp]\n";
#endif
        cinBackup = std::cin.rdbuf();

        buffer = new std::stringbuf();
        std::cin.rdbuf(buffer);
    }

    void TearDown() override {
#if VERBOSE > 1
        std::cout << "[TEST TearDown]\n";
#endif
        int toClear = buffer->in_avail();
        buffer->pubseekoff(toClear, std::ios_base::cur, std::ios_base::out);

        Token tmp;
        for (int i = 0; i < 4; i++) {
#if VERBOSE > 2
            std::cout << "[CLEARING] " << i << '\n';
#endif
            scanner_get_token(&tmp, EOL_OPTIONAL);
        }

        std::cin.rdbuf(cinBackup);
        delete buffer;
    }

    void ComplexTest(std::string &inputStr, std::list<ExpectedToken> &expected);
};

ScannerResult scanner_get_token_wrapper(Token *token, EolRule eolRule, ScannerResult expRes, TokenType expType) {
    auto res = scanner_get_token(token, eolRule);
#if VERBOSE
    std::cout << "[SCANNER] Token. Result: '" << stateNames[res] << "' (exp. '"
              << stateNames[expRes]
              << "'). Type: ["
              << tokenTypeNames[token->type] << "] (exp. [" << tokenTypeNames[expType] << "]).\n";
#endif
    return res;
}

#define LEX(input, eolRule, expectedScannerResult, expectedResult)  \
    std::string inputStr = input;                                   \
    buffer->sputn(inputStr.c_str(), inputStr.length());             \
    buffer->sputc(EOF);                                             \
    Token resultToken;                                              \
    auto res = scanner_get_token_wrapper(&resultToken, (eolRule), (expectedScannerResult), (expectedResult)); \
    ASSERT_EQ(res, (expectedScannerResult));                        \
    ASSERT_EQ(resultToken.type, (expectedResult))

#define LEX_SUCCESS(input, expectedResult) \
    LEX(input, EOL_OPTIONAL, SCANNER_RESULT_SUCCESS, expectedResult)

#define LEX_SUCCESS_EOL(input, eolRule, expectedResult) \
    LEX(input, eolRule, SCANNER_RESULT_SUCCESS, expectedResult)

void ScannerTest::ComplexTest(std::string &inputStr, std::list<ExpectedToken> &expected) {
#if VERBOSE
    std::cout << "[TEST] Input string:\n" << inputStr << "\n[START]\n";
#endif
    buffer->sputn(inputStr.c_str(), inputStr.length());
    buffer->sputc(EOF);

    Token resultToken;

    ScannerResult res;
    EolRule nextEolRule = EOL_OPTIONAL;
    auto it = expected.begin();
    auto end = expected.end();

    while ((res = scanner_get_token(&resultToken, nextEolRule)) != SCANNER_RESULT_EOF) {
        if (it == end) {
#if VERBOSE
            std::cout << "[TEST] Got to the end of expected token list; asserting success.";
#endif
            ASSERT_EQ(res, SCANNER_RESULT_SUCCESS);
            continue;
        }

        auto expectedToken = *it;
#if VERBOSE
        std::cout << "[SCANNER] Token. Result: '" << stateNames[res] << "' (exp. '"
                  << stateNames[expectedToken.expectedResult]
                  << "'). Type: ["
                  << tokenTypeNames[resultToken.type] << "] (exp. [" << tokenTypeNames[expectedToken.type] << "]).\n";
#endif
        ASSERT_EQ(res, expectedToken.expectedResult);
        ASSERT_EQ(resultToken.type, expectedToken.type);

        switch (expectedToken.type) {
            case TOKEN_ID:
                ASSERT_STREQ(mstr_content(&resultToken.data.str_val), expectedToken.data.strVal);
                break;
            case TOKEN_KEYWORD:
                ASSERT_EQ(resultToken.data.keyword_type, expectedToken.data.kw);
                break;
            case TOKEN_INT:
                ASSERT_EQ(resultToken.data.num_int_val, expectedToken.data.intVal);
                break;
            case TOKEN_FLOAT:
                ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, expectedToken.data.doubleVal);
                break;
            case TOKEN_BOOL:
                ASSERT_EQ(resultToken.data.bool_val, expectedToken.data.boolVal);
                break;
            default:
                break;
        }

        nextEolRule = expectedToken.nextEolRule;
        it++;
    }

    ASSERT_EQ(it, end);
}

std::list<ExpectedToken> MakeBasicFunctionResults() {
    return std::list<ExpectedToken>{
            ExpectedToken(TOKEN_KEYWORD, {.kw = KeywordType::KEYWORD_FUNC}, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "add"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_LEFT_BRACKET, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "i"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_KEYWORD, {.kw = KeywordType::KEYWORD_INT}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_COMMA, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "j"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_KEYWORD, {.kw = KeywordType::KEYWORD_INT}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_RIGHT_BRACKET, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_LEFT_BRACKET, EOL_OPTIONAL),
            ExpectedToken(TOKEN_KEYWORD, {.kw = KeywordType::KEYWORD_INT}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_RIGHT_BRACKET, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_CURLY_LEFT_BRACKET, EOL_REQUIRED),
            ExpectedToken(TOKEN_KEYWORD, {.kw = KeywordType::KEYWORD_RETURN}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_ID, {.strVal = "i"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_PLUS, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "j"}, EOL_OPTIONAL),
            ExpectedToken(TOKEN_CURLY_RIGHT_BRACKET, EOL_REQUIRED)
    };
}

TEST_F(ScannerTest, FunctionValid) {
    std::string inputStr = "func add(i int, j int) (int) {\n return i + j }\n";
    auto expectedResult = MakeBasicFunctionResults();

    ComplexTest(inputStr, expectedResult);
}

TEST_F(ScannerTest, FunctionValidOptionalEols) {
    std::string inputStr = "func\nadd(\ni int,\nj int) (\nint) { \n return i +\n j \n }\n";
    auto expectedResult = MakeBasicFunctionResults();

    ComplexTest(inputStr, expectedResult);
}

TEST_F(ScannerTest, FunctionExcessEol) {
    std::string inputStr = "func add\n(i int, j int) (int) { \n return i + j }\n";
    auto expectedResult = MakeBasicFunctionResults();

    auto it = expectedResult.begin();
    it++;
    it++;
    it->expectedResult = SCANNER_RESULT_EXCESS_EOL;

    ComplexTest(inputStr, expectedResult);
}

TEST_F(ScannerTest, FunctionMultipleExcessEols) {
    std::string inputStr = "func add\n(i int, j int) (int) { \n return i \n+ j }\n";
    auto expectedResult = MakeBasicFunctionResults();

    auto it = expectedResult.begin();
    std::next(it, 2)->expectedResult = SCANNER_RESULT_EXCESS_EOL;

    auto it2 = expectedResult.rbegin();
    std::next(it2, 2)->expectedResult = SCANNER_RESULT_EXCESS_EOL;

    ComplexTest(inputStr, expectedResult);
}

TEST_F(ScannerTest, FunctionMissingEol) {
    std::string inputStr = "func add (i int, j int) (int) { return i + j }\n";
    auto expectedResult = MakeBasicFunctionResults();

    auto it = std::next(expectedResult.rbegin(), 4);
    it->expectedResult = SCANNER_RESULT_MISSING_EOL;

    ComplexTest(inputStr, expectedResult);
}

std::list<ExpectedToken> MakeInvocationResults() {
    return std::list<ExpectedToken>{
            ExpectedToken(TOKEN_ID, {.strVal = "i"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_ASSIGN, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "add"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_LEFT_BRACKET, EOL_OPTIONAL),
            ExpectedToken(TOKEN_INT, {.intVal = 1}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_COMMA, EOL_OPTIONAL),
            ExpectedToken(TOKEN_INT, {.intVal = 1}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_RIGHT_BRACKET, EOL_REQUIRED)
    };
}

TEST_F(ScannerTest, InvocationValidOptionalEols) {
    std::string inputStr = "i =\nadd(\n1, \n1)\n";
    auto expectedResult = MakeInvocationResults();

    ComplexTest(inputStr, expectedResult);
}

TEST_F(ScannerTest, InvocationMultipleExcessEols) {
    std::string inputStr = "i\n= add(1, 1\n)\n";
    auto expectedResult = MakeInvocationResults();

    expectedResult.rbegin()->expectedResult = SCANNER_RESULT_EXCESS_EOL;
    std::next(expectedResult.begin(), 1)->expectedResult = SCANNER_RESULT_EXCESS_EOL;

    ComplexTest(inputStr, expectedResult);
}

std::list<ExpectedToken> MakeComparisonsResults() {
    return std::list<ExpectedToken>{
            ExpectedToken(TOKEN_KEYWORD, {.kw = KEYWORD_IF}, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "a"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_GREATER_THAN, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "b"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_CURLY_LEFT_BRACKET, EOL_REQUIRED),
            ExpectedToken(TOKEN_ID, {.strVal = "expr"}, EOL_OPTIONAL),
            ExpectedToken(TOKEN_CURLY_RIGHT_BRACKET, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_KEYWORD, {.kw = KEYWORD_ELSE}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_CURLY_LEFT_BRACKET, EOL_REQUIRED),
            ExpectedToken(TOKEN_KEYWORD, {.kw = KEYWORD_IF}, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "a"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_LESS_OR_EQUAL, EOL_OPTIONAL),
            ExpectedToken(TOKEN_ID, {.strVal = "b"}, EOL_FORBIDDEN),
            ExpectedToken(TOKEN_CURLY_LEFT_BRACKET, EOL_REQUIRED),
            ExpectedToken(TOKEN_ID, {.strVal = "expr"}, EOL_OPTIONAL),
            ExpectedToken(TOKEN_CURLY_RIGHT_BRACKET, EOL_REQUIRED),
            ExpectedToken(TOKEN_ID, {.strVal = "expr"}, EOL_OPTIONAL),
            ExpectedToken(TOKEN_CURLY_RIGHT_BRACKET, EOL_REQUIRED)
    };
}

TEST_F(ScannerTest, ComparisonsValidOptionalEols) {
    std::string inputStr = "if\na > \nb {\nexpr\n} else {\n if a <= b {\n expr }\n expr }\n";

    auto expectedResult = MakeComparisonsResults();
    ComplexTest(inputStr, expectedResult);
}

// Tests whether an input with no \n at its end still passes the EOL_REQUIRED rule and terminates correctly,
// with SCANNER_RESULT_EOF instead of SCANNER_RESULT_MISSING_EOL.
TEST_F(ScannerTest, NoRequiredEolBeforeEof) {
    LEX("}", EOL_OPTIONAL, SCANNER_RESULT_SUCCESS, TOKEN_CURLY_RIGHT_BRACKET);
    res = scanner_get_token(&resultToken, EOL_REQUIRED);
    ASSERT_EQ(res, SCANNER_RESULT_EOF);
}

// Tests whether an input with no \n at its end terminates correctly.
TEST_F(ScannerTest, NoOptionalEolBeforeEof) {
    LEX("}", EOL_OPTIONAL, SCANNER_RESULT_SUCCESS, TOKEN_CURLY_RIGHT_BRACKET);
    res = scanner_get_token(&resultToken, EOL_OPTIONAL);
    ASSERT_EQ(res, SCANNER_RESULT_EOF);
}

TEST_F(ScannerTest, IdentifierLowercase) {
    LEX_SUCCESS("abcd", TOKEN_ID);
    ASSERT_STREQ("abcd", mstr_content(&resultToken.data.str_val));
}

TEST_F(ScannerTest, IdentifierUppercase) {
    LEX_SUCCESS("ABCD", TOKEN_ID);
    ASSERT_STREQ("ABCD", mstr_content(&resultToken.data.str_val));
}

TEST_F(ScannerTest, IdentifierMixed) {
    LEX_SUCCESS("AbCd", TOKEN_ID);
    ASSERT_STREQ("AbCd", mstr_content(&resultToken.data.str_val));
}

TEST_F(ScannerTest, IdentifierMixedNumbers) {
    LEX_SUCCESS("AbCd123", TOKEN_ID);
    ASSERT_STREQ("AbCd123", mstr_content(&resultToken.data.str_val));
}

TEST_F(ScannerTest, IdentifierUnderscore) {
    LEX_SUCCESS("_abc", TOKEN_ID);
    ASSERT_STREQ("_abc", mstr_content(&resultToken.data.str_val));
}

TEST_F(ScannerTest, IdentifierMixedUnderscore) {
    LEX_SUCCESS("_ab_c", TOKEN_ID);
    ASSERT_STREQ("_ab_c", mstr_content(&resultToken.data.str_val));
}

TEST_F(ScannerTest, IdentifierMixedUnderscoreNumbers) {
    LEX_SUCCESS("_ab_c65", TOKEN_ID);
    ASSERT_STREQ("_ab_c65", mstr_content(&resultToken.data.str_val));
}

TEST_F(ScannerTest, Int) {
    LEX_SUCCESS("1 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 1);
}

TEST_F(ScannerTest, IntZero) {
    LEX_SUCCESS("0 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 0);
}

TEST_F(ScannerTest, IntBinary) {
    LEX_SUCCESS("0b1 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 1);
}

TEST_F(ScannerTest, IntBinaryMultiple) {
    LEX_SUCCESS("0b11 0b0101 0b01000 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 3);

    res = scanner_get_token(&resultToken, EOL_OPTIONAL);
    ASSERT_EQ(res, SCANNER_RESULT_SUCCESS);
    ASSERT_EQ(resultToken.type, TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 5);

    res = scanner_get_token(&resultToken, EOL_OPTIONAL);
    ASSERT_EQ(res, SCANNER_RESULT_SUCCESS);
    ASSERT_EQ(resultToken.type, TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 8);
}

TEST_F(ScannerTest, IntBinaryUpperCase) {
    LEX_SUCCESS("0B10 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 2);
}

TEST_F(ScannerTest, IntOctal) {
    LEX_SUCCESS("0o1 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 1);
}

TEST_F(ScannerTest, IntOctalMultiple) {
    LEX_SUCCESS("0o11 0o001 0o01000 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 9);

    res = scanner_get_token(&resultToken, EOL_OPTIONAL);
    ASSERT_EQ(res, SCANNER_RESULT_SUCCESS);
    ASSERT_EQ(resultToken.type, TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 1);

    res = scanner_get_token(&resultToken, EOL_OPTIONAL);
    ASSERT_EQ(res, SCANNER_RESULT_SUCCESS);
    ASSERT_EQ(resultToken.type, TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 512);
}

TEST_F(ScannerTest, IntOctalUpperCase) {
    LEX_SUCCESS("0O10 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 8);
}

TEST_F(ScannerTest, IntHexa) {
    LEX_SUCCESS("0x1 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 1);
}

TEST_F(ScannerTest, IntHexaMultiple) {
    LEX_SUCCESS("0x11 0x001 0x01000 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 0x11);

    res = scanner_get_token(&resultToken, EOL_OPTIONAL);
    ASSERT_EQ(res, SCANNER_RESULT_SUCCESS);
    ASSERT_EQ(resultToken.type, TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 1);

    res = scanner_get_token(&resultToken, EOL_OPTIONAL);
    ASSERT_EQ(res, SCANNER_RESULT_SUCCESS);
    ASSERT_EQ(resultToken.type, TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 0x01000);
}

TEST_F(ScannerTest, IntHexaLetters) {
    LEX_SUCCESS("0x1abcd ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 0x1abcd);
}

TEST_F(ScannerTest, IntHexaLettersUpperCase) {
    LEX_SUCCESS("0x1ABCD ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 0x1abcd);
}

TEST_F(ScannerTest, IntHexaLettersMixed) {
    LEX_SUCCESS("0x1AbCdEfaBcDeF ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 0x1abcdefabcdef);
}

TEST_F(ScannerTest, IntHexaUpperCase) {
    LEX_SUCCESS("0X10 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 16);
}

TEST_F(ScannerTest, IntMultiple) {
    LEX_SUCCESS("123456 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 123456);
}

TEST_F(ScannerTest, IntTooBig) {
    LEX("9223372036854775809 ", EOL_OPTIONAL,
        SCANNER_RESULT_NUMBER_OVERFLOW, TOKEN_INT);
}

TEST_F(ScannerTest, IntMultipleWithZero) {
    LEX_SUCCESS("103456 ", TOKEN_INT);
    ASSERT_EQ(resultToken.data.num_int_val, 103456);
}

TEST_F(ScannerTest, IntExp) {
    LEX_SUCCESS("16e1 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 160.0);
}

TEST_F(ScannerTest, IntExpUpperCase) {
    LEX_SUCCESS("16E1 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 160.0);
}

TEST_F(ScannerTest, IntExpZero) {
    LEX_SUCCESS("16e0 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0);
}

TEST_F(ScannerTest, IntExpMultiple) {
    LEX_SUCCESS("16e10 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0e10);
}

TEST_F(ScannerTest, IntExpMultipleZero) {
    LEX_SUCCESS("16e02 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1600.0);
}

TEST_F(ScannerTest, IntExpPlusSign) {
    LEX_SUCCESS("16e+1 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 160.0);
}

TEST_F(ScannerTest, IntExpPlusSignZero) {
    LEX_SUCCESS("16e+0 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0);
}

TEST_F(ScannerTest, IntExpMinusSign) {
    LEX_SUCCESS("160e-1 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0);
}

TEST_F(ScannerTest, IntExpMinusSignZero) {
    LEX_SUCCESS("16e-0 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0);
}

TEST_F(ScannerTest, IntExpPlusSignMultiple) {
    LEX_SUCCESS("16e+10 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16e10);
}

TEST_F(ScannerTest, IntExpPlusSignMultipleZero) {
    LEX_SUCCESS("16e+010 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16e10);
}

TEST_F(ScannerTest, IntExpMinusSignMultiple) {
    LEX_SUCCESS("1600000e-10 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1600000e-10);
}

TEST_F(ScannerTest, IntExpMinusSignMultipleZero) {
    LEX_SUCCESS("1600000e-010 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1600000e-10);
}

TEST_F(ScannerTest, Float) {
    LEX_SUCCESS("1.5 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1.5);
}

TEST_F(ScannerTest, FloatMultiple) {
    LEX_SUCCESS("1.525 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1.525);
}

TEST_F(ScannerTest, FloatZero) {
    LEX_SUCCESS("1.0 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1.0);
}

TEST_F(ScannerTest, FloatStartingWithZero) {
    LEX_SUCCESS("0.5 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 0.5);
}

TEST_F(ScannerTest, FloatStartingWithZeroDecimal) {
    LEX_SUCCESS("1.025 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1.025);
}

TEST_F(ScannerTest, FloatStartingWithZeroAndZeroDecimal) {
    LEX_SUCCESS("0.025 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 0.025);
}

TEST_F(ScannerTest, FloatExp) {
    LEX_SUCCESS("1.0e1 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1.0e1);
}

TEST_F(ScannerTest, FloatExpUpperCase) {
    LEX_SUCCESS("1.0E1 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1.0e1);
}

TEST_F(ScannerTest, FloatExpZero) {
    LEX_SUCCESS("1.0e0 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1.0);
}

TEST_F(ScannerTest, FloatExpMultiple) {
    LEX_SUCCESS("16.0e10 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0e10);
}

TEST_F(ScannerTest, FloatExpMultipleZero) {
    LEX_SUCCESS("16.0e02 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1600.0);
}

TEST_F(ScannerTest, FloatExpPlusSign) {
    LEX_SUCCESS("16.0e+1 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 160.0);
}

TEST_F(ScannerTest, FloatExpPlusSignZero) {
    LEX_SUCCESS("16.0e+0 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0);
}

TEST_F(ScannerTest, FloatExpMinusSign) {
    LEX_SUCCESS("160.0e-1 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0);
}

TEST_F(ScannerTest, FloatExpMinusSignZero) {
    LEX_SUCCESS("16.0e-0 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0);
}

TEST_F(ScannerTest, FloatExpPlusSignMultiple) {
    LEX_SUCCESS("16.0e+10 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0e10);
}

TEST_F(ScannerTest, FloatExpPlusSignMultipleZero) {
    LEX_SUCCESS("16.0e+010 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 16.0e10);
}

TEST_F(ScannerTest, FloatExpMinusSignMultiple) {
    LEX_SUCCESS("1600000.0e-10 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1600000.0e-10);
}

TEST_F(ScannerTest, FloatExpMinusSignMultipleZero) {
    LEX_SUCCESS("1600000.0e-010 ", TOKEN_FLOAT);
    ASSERT_DOUBLE_EQ(resultToken.data.num_float_val, 1600000.0e-10);
}

TEST_F(ScannerTest, StringEmpty) {
    LEX_SUCCESS("\"\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "");
}

TEST_F(ScannerTest, StringOneChar) {
    LEX_SUCCESS("\"a\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "a");
}

TEST_F(ScannerTest, StringOneCharUppercase) {
    LEX_SUCCESS("\"A\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "A");
}

TEST_F(ScannerTest, StringMultipleChars) {
    LEX_SUCCESS("\"aBcDe\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "aBcDe");
}

TEST_F(ScannerTest, StringOneNumber) {
    LEX_SUCCESS("\"1\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "1");
}

TEST_F(ScannerTest, StringBslN) {
    LEX_SUCCESS("\"\\n\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\n");
}

TEST_F(ScannerTest, StringBslT) {
    LEX_SUCCESS("\"\\t\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\t");
}

TEST_F(ScannerTest, StringBslQuotes) {
    LEX_SUCCESS("\"\\\"\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\"");
}

TEST_F(ScannerTest, StringHexa) {
    LEX_SUCCESS("\"\\xaa\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\xAA");
}

TEST_F(ScannerTest, StringHexaUpperCase) {
    LEX_SUCCESS("\"\\xAA\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\xAA");
}

TEST_F(ScannerTest, StringHexaMixedCase) {
    LEX_SUCCESS("\"\\xAb\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\xAB");
}

TEST_F(ScannerTest, StringHexaNumeric) {
    LEX_SUCCESS("\"\\x32\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\x32");
}

TEST_F(ScannerTest, StringHexaMultiple) {
    LEX_SUCCESS("\"\\xaa\\x32\\x3A\\xFa\" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\xAA\x32\x3A\xFA");
}

TEST_F(ScannerTest, StringSpace) {
    LEX_SUCCESS("\" \" ", TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), " ");
}

TEST_F(ScannerTest, StringComplex) {
    LEX_SUCCESS("\"\\t\\\"Prilis zlutoucky \\x6Bun upel da\\x62elske \\x6fdy!\\\"\" ",
                TOKEN_STRING);
    ASSERT_STREQ(mstr_content(&resultToken.data.str_val), "\t\"Prilis zlutoucky kun upel dabelske ody!\"");
}

TEST_F(ScannerTest, KeywordBool) {
    LEX_SUCCESS("bool a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_BOOL);
}

TEST_F(ScannerTest, KeywordElse) {
    LEX_SUCCESS("else a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_ELSE);
}

TEST_F(ScannerTest, KeywordFloat) {
    LEX_SUCCESS("float64 a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_FLOAT64);
}

TEST_F(ScannerTest, KeywordFor) {
    LEX_SUCCESS("for a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_FOR);
}

TEST_F(ScannerTest, KeywordFunc) {
    LEX_SUCCESS("func a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_FUNC);
}

TEST_F(ScannerTest, KeywordIf) {
    LEX_SUCCESS("if a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_IF);
}

TEST_F(ScannerTest, KeywordInt) {
    LEX_SUCCESS("int a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_INT);
}

TEST_F(ScannerTest, KeywordPackage) {
    LEX_SUCCESS("package a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_PACKAGE);
}

TEST_F(ScannerTest, KeywordReturn) {
    LEX_SUCCESS("return a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_RETURN);
}

TEST_F(ScannerTest, KeywordString) {
    LEX_SUCCESS("string a", TOKEN_KEYWORD);
    ASSERT_EQ(resultToken.data.keyword_type, KeywordType::KEYWORD_STRING);
}

TEST_F(ScannerTest, KeywordTrue) {
    LEX_SUCCESS("true a", TOKEN_BOOL);
    ASSERT_EQ(resultToken.data.bool_val, true);
}

TEST_F(ScannerTest, KeywordFalse) {
    LEX_SUCCESS("false a", TOKEN_BOOL);
    ASSERT_EQ(resultToken.data.bool_val, false);
}

TEST_F(ScannerTest, TokenPlus) {
    LEX_SUCCESS("+ a", TOKEN_PLUS);
}

TEST_F(ScannerTest, TokenMinus) {
    LEX_SUCCESS("- a", TOKEN_MINUS);
}

TEST_F(ScannerTest, TokenMultiply) {
    LEX_SUCCESS("* a", TOKEN_MULTIPLY);
}

TEST_F(ScannerTest, TokenDivide) {
    LEX_SUCCESS("/ a", TOKEN_DIVIDE);
}

TEST_F(ScannerTest, TokenPlusAssign) {
    LEX_SUCCESS("+= a", TOKEN_PLUS_ASSIGN);
}

TEST_F(ScannerTest, TokenMinusAssign) {
    LEX_SUCCESS("-= a", TOKEN_MINUS_ASSIGN);
}

TEST_F(ScannerTest, TokenMultiplyAssign) {
    LEX_SUCCESS("*= a", TOKEN_MULTIPLY_ASSIGN);
}

TEST_F(ScannerTest, TokenDivideAssign) {
    LEX_SUCCESS("/= a", TOKEN_DIVIDE_ASSIGN);
}

TEST_F(ScannerTest, TokenDefine) {
    LEX_SUCCESS(":= a", TOKEN_DEFINE);
}

TEST_F(ScannerTest, TokenAssign) {
    LEX_SUCCESS("= a", TOKEN_ASSIGN);
}

TEST_F(ScannerTest, TokenEqualTo) {
    LEX_SUCCESS("== a", TOKEN_EQUAL_TO);
}

TEST_F(ScannerTest, TokenNot) {
    LEX_SUCCESS("! a", TOKEN_NOT);
}

TEST_F(ScannerTest, TokenNotEqualTo) {
    LEX_SUCCESS("!= a", TOKEN_NOT_EQUAL_TO);
}

TEST_F(ScannerTest, TokenAnd) {
    LEX_SUCCESS("&& a", TOKEN_AND);
}

TEST_F(ScannerTest, TokenOr) {
    LEX_SUCCESS("|| a", TOKEN_OR);
}

TEST_F(ScannerTest, TokenLeftBracket) {
    LEX_SUCCESS("( a", TOKEN_LEFT_BRACKET);
}

TEST_F(ScannerTest, TokenRightBracket) {
    LEX_SUCCESS(") a", TOKEN_RIGHT_BRACKET);
}

TEST_F(ScannerTest, TokenLeftCurlyBracket) {
    LEX_SUCCESS("{ a", TOKEN_CURLY_LEFT_BRACKET);
}

TEST_F(ScannerTest, TokenRightCurlyBracket) {
    LEX_SUCCESS("} a", TOKEN_CURLY_RIGHT_BRACKET);
}

TEST_F(ScannerTest, TokenLessThan) {
    LEX_SUCCESS("< a", TOKEN_LESS_THAN);
}

TEST_F(ScannerTest, TokenGreaterThan) {
    LEX_SUCCESS("> a", TOKEN_GREATER_THAN);
}

TEST_F(ScannerTest, TokenLessOrEqual) {
    LEX_SUCCESS("<= a", TOKEN_LESS_OR_EQUAL);
}

TEST_F(ScannerTest, TokenGreaterOrEqual) {
    LEX_SUCCESS(">= a", TOKEN_GREATER_OR_EQUAL);
}

TEST_F(ScannerTest, TokenComma) {
    LEX_SUCCESS(", a", TOKEN_COMMA);
}

TEST_F(ScannerTest, TokenSemicolon) {
    LEX_SUCCESS("; a", TOKEN_SEMICOLON);
}
