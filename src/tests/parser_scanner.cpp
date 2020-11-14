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
#define VERBOSE 1

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

TEST_F(ParserScannerTest, EOLMissingMainFunc) {
    std::string inputStr = \
        "package main"
        "func main() {\n"
        "    if a {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError1) {
    std::string inputStr = \
        "package main "
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

TEST_F(ParserScannerTest, EmptyLineEOLAfterPackageMissingAllowed) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EmptyLineEOLInFrontOfPackageAllowed) {
    std::string inputStr = \
        "\n"
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
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
}

TEST_F(ParserScannerTest, FunctionMainDefinitionMissing2) {
    std::string inputStr = \
        "package main\n"
        "\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}*/

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

TEST_F(ParserScannerTest, AssignEOL2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a,\n b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AssignEOL3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b\n = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOL4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b =\n 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AssignEOL5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b = 1\n, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOL6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b = 1,\n 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
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

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
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

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
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

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
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

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
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
        "    foo(a, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo\n(a, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a \n, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(\na, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}


TEST_F(ParserScannerTest, TestEOLInFunctionCall5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a, \nc )\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a, c\n )\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a, c string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TestEOLInFunctionCall9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int, c string)"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test return command format ===

TEST_F(ParserScannerTest, ReturnFormat1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    return\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}


TEST_F(ParserScannerTest, ReturnFormat2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    return"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() int {\n"
        "    return a\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

//TODO semantic analyze error
/*
TEST_F(ParserScannerTest, ReturnFormat4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    return a\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}*/

TEST_F(ParserScannerTest, ReturnFormat5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    return \n a\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() (int, float64) {\n"
        "    return a, b\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() (int, string, float64) {\n"
        "    return a, b, c\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() (int, string, float64) {\n"
        "    return a, b\n, c\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (int) {\n"
        "    return 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (\nint) {\n"
        "    return 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (int)\n {\n"
        "    return 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (int, string) {\n"
        "    return 5, \"returned \\xaf string\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (int, string) {\n"
        "    return 5, \"returned \n \\xaf string\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
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

// === Test function definition format ===

TEST_F(ParserScannerTest, FunctionDefinition1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionDefinition2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i int, s string, f float64) (int, string, float64) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionDefinition3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (i int, s string, f float64) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionDefinition4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i int, s string, f float64) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionDefinition5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i in) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionDefinition6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i int, s strng) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionDefinition7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (i in) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionDefinition8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (i int, s strng) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test combining token because of missing EOL ===

TEST_F(ParserScannerTest, CombiningTokensMissingEOL1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (i int, s strng) {\n"
        "    a := foo"
        "    bar = 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Complex functions tests ===

TEST_F(ParserScannerTest, ComplexFunctions1) {
    std::string inputStr = \
        "// Program 1: Vypocet faktorialu (iterativne)\n"
        "package main\n"
        "func main() {\n"
        "    print(\"Zadejte cislo pro vypocet faktorialu: \")\n"
        "    a := 0\n"
        "    a, _ = 45\n"
        "    if 0 {\n"
        "        print(\"Faktorial nejde spocitat!\\n\")\n"
        "    } else {\n"
        "        vysl := 1\n"
        "        for bdav := 1 ; 0; a = 1 {\n"
        "            vysl = vysl\n"
        "        }\n"
        "        print(\"Vysledek je \", vysl, \"\\n\")\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ComplexFunctions2) {
    std::string inputStr = \
        "// Program 3: Prace s retezci a vestavenymi funkcemi\n"
        "package main\n"
        "func main() {\n"
        "    s1 := \"Toto je nejaky text\"\n"
        "    s2 := a\n"
        "    print(s1, \"\\n\", s2)\n"
        "    s1len := 0\n"
        "    s1len = a\n"
        "    s1len = b\n"
        "    s1, _ = 5, 4\n"
        "    s1len = 4\n"
        "    print(\"4 znaky od\", s1len, \". znaku v \\\"\", s2, \"\\\":\", s1, \"\\n\")\n"
        "    print(\"Zadejte serazenou posloupnost vsech malych pismen a-h, \")\n"
        "    print(\"pricemz se pismena nesmeji v posloupnosti opakovat: \")\n"
        "    err := 0\n"
        "    s1, err = w\n"
        "    if 1 {\n"
        "        for ; a; {\n"
        "            print(\"\\n\", \"Spatne zadana posloupnost, zkuste znovu: \")\n"
        "            s1, _ = 4, 4\n"
        "        }\n"
        "    } else {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ComplexFunctions3) {
    std::string inputStr = \
        "// Program 3: Prace s retezci a vestavenymi funkcemi\n"
        "package main\n"
        "func main() {\n"
        "    s1 := \"Toto je nejaky text\"\n"
        "    s2 := a\n"
        "    print(s1, \"\\n\", s2)\n"
        "    s1len := 0\n"
        "    s1len = a\n"
        "    s1len = b\n"
        "    s1, _ = 5, 4\n"
        "    s1len = 4\n"
        "    print(\"4 znaky od\", s1len, \". znaku v \\\"\", s2, \"\\\":\", s1, \"\\n\")\n"
        "    print(\"Zadejte serazenou posloupnost vsech malych pismen a-h, \")\n"
        "    print(\"pricemz se pismena nesmeji v posloupnosti opakovat: \")\n"
        "    err := 0\n"
        "    s1, err = w\n"
        "    if 1 {\n"
        "        for ; a; {\n"
        "            print(\"\\n\", \"Spatne zadana posloupnost, zkuste znovu: \")\n"
        "            s1, _ = 4, 4\n"
        "        }\n"
        "    } else {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Test on long idetifiers ===

TEST_F(ParserScannerTest, LongIdentifiers) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo() (i int, s string) {\n"
        "    a := LongIndetifier_djlajdlawjdlajwdjawdklawjdlawjkldawjdjawdjawkljdklawjdkljawkldjawdjawdjawkldjawdlajdawkldawkldjawdlawkldawjdawjdawjdklawjdaw\n"
        "    LongIndetifier_ahdwjlahwdjawhldahwdawhdawlhdawlkhdklawhdioawiodhaiowdhahdwhawiodhiawhdhawidhawiodhahwdiohawiodhiawdhioawhdawhiodawhidhawdohawihdiawdawd = 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Others ===

TEST_F(ParserScannerTest, EmptyMainFunction1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n";
    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EmptyMainFunction2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {}\n";
    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EmptyRandomFunction) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n";
        "func foo() {\n"
        "}\n";
    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}
