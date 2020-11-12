/** @file parser_scanner.cpp
 *
 * IFJ20 compiler tests
 *
 * @brief Contains tests for the parser and scanner modules.
 *
 * @author David Chocholatý (xchoch08), FIT BUT
 */

// Verbosity levels:
// 0 – no debug info
// 1 – info about processed tokens
// 2 – all info excluding character reads and buffer clearing
// 3 – all info
#define VERBOSE 2

#include <iostream>
#include <array>
#include "gtest/gtest.h"

extern "C" {
#include "scanner.h"
#include "parser.h"
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



class ParserScannerTest : public ::testing::Test {
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

    void ComplexTest(std::string &inputStr, CompilerResult expected_compiler_result);
};

void ParserScannerTest::ComplexTest(std::string &inputStr, CompilerResult expected_compiler_result) {
#if VERBOSE
    std::cout << "[TEST] Input string:\n" << inputStr << "\n[START]\n";
#endif
    buffer->sputn(inputStr.c_str(), inputStr.length());
    buffer->sputc(EOF);

    ASSERT_EQ(parser_parse(), expected_compiler_result);
}




TEST_F(ParserScannerTest, BasicCode) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 0\n"
        "    a -= 5\n"
        "\n"
        "    if 1 {\n"
        "        print(\"Will always be printed.\\n\")\n"
        "    } else {\n"
        "        print(\"Will never be seen.\n\")\n"
        "    }\n"
        "    \n"
        "    return\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, MinimalisticFunction) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 0\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, MinimalisticIfFunction) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a := 0 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, PackageTypo) {
    std::string inputStr = \
        "pckage main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError1) {
    std::string inputStr = \
        "package main"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {"
        "    if a {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {"
        "    }"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        if \"cond\" {"
        "           a == 5\n"
        "        }\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        if \"cond\" {\n"
        "           a == 5"
        "        }\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        if 0.5 {\n"
        "            a = \"String\\n\"\n"
        "        }"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        if 0.5 {\n"
        "            a = \"String\\n\"\n"
        "            b := 545.4e-10"
        "        }\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, LastEOLIsNotRequired) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        if 0.5 {\n"
        "            a = \"String\\n\"\n"
        "            b := 545.4e-10\n"
        "        }\n"
        "    }\n"
        "}";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EmptyLineEOLAfterPackageMissing) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EmptyLineEOLInFrontOfPackageForbidden) {
    std::string inputStr = \
        "\n"
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}


TEST_F(ParserScannerTest, EOLInForLoop1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; b; c {"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoop2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; b; c {\n"
        "    }"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoop3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5;\n b; c {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for\n a; b; c {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a\n; b; c {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; b\n; c {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; b;\n c {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; b; c\n {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; b; c {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFuncDefinitionForbidden1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func \nmain() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFuncDefinitionForbidden2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main \n () {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFuncDefinitionForbidden3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main(\n) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFuncDefinitionForbidden4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() \n{\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFuncDefinitionMissing) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Package main missing ===

TEST_F(ParserScannerTest, PackageMainMissing1) {
    std::string inputStr = \
        "\n"
        "func main() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, PackageMainMissing2) {
    std::string inputStr = \
        "func main() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Function main definition missing ===
//TODO uncomment when implemented
/*
TEST_F(ParserScannerTest, FunctionMainDefinitionMissing1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func not_main() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}*/

TEST_F(ParserScannerTest, FunctionMainDefinitionMissing2) {
    std::string inputStr = \
        "package main\n"
        "\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Incomplete function definition ===

TEST_F(ParserScannerTest, IncompleteFunctionDefinition1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        " main() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, IncompleteFunctionDefinition2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func () {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, IncompleteFunctionDefinition3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, IncompleteFunctionDefinition4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() \n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, IncompleteFunctionDefinition5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Assing commmand EOLs tests ===

TEST_F(ParserScannerTest, AssignEOLMissing) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b = 1, 2"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOLForbidden1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a\n, b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOLForbidden2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a,\n b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOLForbidden3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b\n = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOLForbidden4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b =\n 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOLForbidden5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b = 1\n, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOLForbidden6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b = 1,\n 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test EOL in commands ===

TEST_F(ParserScannerTest, TestEOLInDefinitionOfVariable1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a\n := 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInDefinitionOfVariable2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a :=\n 12\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInDefinitionOfVariable3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b \n := 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInDefinitionOfVariable4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := 1\n, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInDefinitionOfVariable5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := 1,\n 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInDefinitionOfVariable6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, _ := 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, TestEOLInDefinitionOfVariable7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _\n, a  := 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test EOL in if-else if-else costruction ===

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if \na {\n"
        "        foo = 42\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a \n{\n"
        "        foo = 42\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } \n else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if na {\n"
        "        foo = 42\n"
        "    } else \n {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    \n\n\n\n"
        "} else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "    \n\n\n\n"
        "        foo = 42\n"
        "} else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {"
        "        foo = 42\n"
        "} else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInIfElseIfElseConstrcuction9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "} else {"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test EOL in function call ===

TEST_F(ParserScannerTest, TestEOLInFunctionCall1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int, c string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo\n(a int, c string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a \nint, c string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(\na int, c string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a \nint, c string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int\n, c string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int, \nc string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int, c\n string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int, c string\n)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int, c string)"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}



















// === Insert more whitespaces ===

TEST_F(ParserScannerTest, InsertMoreWhitespaces1) {
    std::string inputStr = \
        "package     main\n"
        "  \n"
        "func main(   )    {\n"
        "    a    , b        = 1,     2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, InsertMoreTabs) {
    std::string inputStr = \
        "package\tmain\n"
        "\t\n"
        "func main(\t) {\n"
        "a\t, b = 1,\t\t\t2\n"
        "\t\t\t}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments1) {
    std::string inputStr = \
        "package main\n"
        "\n /* comment to be skipped */"
        "func main() {\n"
        "    a, b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() { // one-line comment \n"
        "    a, b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() { /* multi-line comment \n \n about stuff */ \n"
        "    a, b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b = /* little in-command comment */ 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}
