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
#include "stdin_mock_test.h"

extern "C" {
#include "scanner.h"
#include "parser.h"
#include "mutable_string.h"
}

class ParserScannerTest : public StdinMockingScannerTest {
protected:
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
        "    if 0 < 1 {\n"
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

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a\n := 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a :=\n 12\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b \n := 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := 1\n, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := 1,\n 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, _ := 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _\n, a  := 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test EOL in if-else statements ===

TEST_F(ParserScannerTest, EOLInIfElseStatement1) {
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

TEST_F(ParserScannerTest, EOLInIfElseStatement2) {
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

TEST_F(ParserScannerTest, EOLInIfElseStatement3) {
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

TEST_F(ParserScannerTest, EOLInIfElseStatement4) {
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

TEST_F(ParserScannerTest, EOLInIfElseStatement5) {
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

TEST_F(ParserScannerTest, EOLInIfElseStatement6) {
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

TEST_F(ParserScannerTest, EOLInIfElseStatement7) {
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

TEST_F(ParserScannerTest, EOLInIfElseStatement8) {
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

TEST_F(ParserScannerTest, EOLInIfElseStatement9) {
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

// === Test EOL in if/else if(/else) statements ===

TEST_F(ParserScannerTest, EOLInIfElseIfElseStatement1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInIfElseIfElseStatement2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } else if \nb {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInIfElseIfElseStatement3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } else if b\n {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInIfElseIfElseStatement4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } \n else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInIfElseIfElseStatement5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } else if b {"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInIfElseIfElseStatement6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } else if {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } else if b \n {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a {\n"
        "        foo = 42\n"
        "    } else if \n b {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if\n a {\n"
        "        foo = 42\n"
        "    } else if \n b {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a \n{\n"
        "        foo = 42\n"
        "    } else if \n b {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test EOL in function call ===

TEST_F(ParserScannerTest, EOLInFunctionCall1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo\n(a, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a \n, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(\na, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}


TEST_F(ParserScannerTest, EOLInFunctionCall5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a, \nc )\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a, c\n )\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a int, c)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(a, c string)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall9) {
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

TEST_F(ParserScannerTest, ReturnFormat4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    return \n a\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() (int, float64) {\n"
        "    return a, b\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() (int, string, float64) {\n"
        "    return a, b, c\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() (int, string, float64) {\n"
        "    return a, b\n, c\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat8) {
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

TEST_F(ParserScannerTest, ReturnFormat9) {
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

TEST_F(ParserScannerTest, ReturnFormat10) {
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

TEST_F(ParserScannerTest, ReturnFormat11) {
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

TEST_F(ParserScannerTest, ReturnFormat12) {
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
        "    a, _ = 45, 0\n"
        "    if 0 > -1 {\n"
        "        print(\"Faktorial nejde spocitat!\\n\")\n"
        "    } else {\n"
        "        vysl := 1\n"
        "        for bdav := 1 ; bdav < 10; a += 1 {\n"
        "            vysl = vysl + 8\n"
        "        }\n"
        "        print(\"Vysledek je \", vysl, \"\\n\")\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ComplexFunctions2) {
    std::string inputStr = \
        "// Program 2: Vypocet faktorialu (rekurzivne)\n"
        "package main\n"
        "func factorial(n int) (int) {\n"
        "   dec_n := n - 1\n"
        "   if n < 2 {\n"
        "       return 1\n"
        "   } else {\n"
        "       tmp := 0\n"
        "       tmp = factorial(dec_n)\n"
        "       return n * tmp\n"
        "   }\n"
        "}\n"
        "func main() {\n"
        "   print(\"Zadejte cislo pro vypocet faktorialu: \")\n"
        "   a := 0\n"
        "   err := 0\n"
        "   a, err = inputi()\n"
        "   if err == 0 {\n"
        "       if a < 0 {\n"
        "           print(\"Faktorial nejde spocitat!\", \"\\n\")\n"
        "       } else {\n"
        "           vysl := 0\n"
        "           vysl = factorial(a)\n"
        "           print(\"Vysledek je \", vysl, \"\\n\")\n"
        "       }\n"
        "   } else {\n"
        "       print(\"Chyba pri nacitani celeho cisla!\\n\")\n"
        "   }\n"
        "}";

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
        "    if true {\n"
        "        for ; a < 10; {\n"
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

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Test function headers ===

TEST_F(ParserScannerTest, FunctionHeader1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo(i int, s string) (bool) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionHeader2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo(i int, s string) (bool, int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionHeader3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo() bool, int {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo(i int, s string) bar(bool, int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo(i int, s string) bool {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionHeader6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo(i int, s string) (bool, int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionHeader7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo i int, s string (bool, int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo(i int s string) (bool, int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test compiler result lexical error ===

TEST_F(ParserScannerTest, LexicalErrorCompilerResult1) {
    std::string inputStr = \
        ">package main\n"
        "\n"
        "func main() {\n"
        "}\n";

   ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, LexicalErrorCompilerResult2) {
    std::string inputStr = \
        "^package main\n"
        "\n"
        "func main() {\n"
        "}\n";

   ComplexTest(inputStr, COMPILER_RESULT_ERROR_LEXICAL);
}

TEST_F(ParserScannerTest, LexicalErrorCompilerResult3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"foo\\sbar\" \n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
    ASSERT_EQ(compiler_result, COMPILER_RESULT_ERROR_LEXICAL);
}

TEST_F(ParserScannerTest, LexicalErrorCompilerResult4) {
    std::string inputStr = \
        "pa]ckage main\n"
        "\n"
        "func main() {\n"
        " a := \"foo\\sbar\" \n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, LexicalErrorCompilerResult5) {
    std::string inputStr = \
        "]package main\n"
        "\n"
        "func main() {\n"
        " a := \"foo\\sbar\" \n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_LEXICAL);
}

// === Test function call ===

TEST_F(ParserScannerTest, FunctionCall1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "   foo(a, b)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "   foo(5+4)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "   foo(true)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a = foo()\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := foo()\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a = foo(4, 5)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a = foo(true, 7, 5.4)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, b = foo()\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, b = foo(7, 4, 1)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _ = foo(7 + 4, 3 + 1)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " _, _ = foo()\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, FunctionCall12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = foo(a && b, 4 + 5 / 7)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = foo(a + 5 && 4 + b)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall14) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = foo((a * 7 - 4) && (b - 9 * (-8)), \"test\")\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall15) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = foo((a))\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall16) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = foo((a && b))\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall17) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = 5 + foo(a && b)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall18) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = 5 / foo(a && b)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall19) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = !foo(a && b)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall20) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a, _, _ = +5 - (-5) / !foo(a && b)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Test basic expression parsing ===

TEST_F(ParserScannerTest, BasicExpressionParsing1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 + 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, BasicExpressionParsing2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 - 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, BasicExpressionParsing3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 * 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, BasicExpressionParsing4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 / 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, BasicExpressionParsing5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 = 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, BasicExpressionParsing6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 -+ 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, BasicExpressionParsing7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 */ 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, BasicExpressionParsing8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 := 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test EOLs in expression ===

TEST_F(ParserScannerTest, EOLInExpression1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 +\n 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInExpression2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a :=\n 5 + 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInExpression3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5\n + 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInExpression4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 5 + 2\n + 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInExpression5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := \n foo()\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Test logical operators ===

TEST_F(ParserScannerTest, LogicalOperators1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := true && false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, LogicalOperators2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := true || false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, LogicalOperators3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := !true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, LogicalOperators4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := !true && false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, LogicalOperators5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := true && !false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, LogicalOperators6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := !false && !true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, LogicalOperators7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := false == true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, LogicalOperators8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := false != true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, LogicalOperators9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := !false != !true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Test unary operators ===

TEST_F(ParserScannerTest, UnaryOperators1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := -5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := +5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := +-5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := -+5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := +(-5)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := +(+5)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := -(+5)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := -(-5)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := +(-5)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := --5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := ++5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := true && !!false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, UnaryOperators13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := true && !!!false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Test expressions in for loops ===

TEST_F(ParserScannerTest, ForLoops1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := 0; i < 10; i += 1 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ForLoops2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i, y := 0, 1; i < 10 && y < 11; i += 1 {\n"
        "        y += 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ForLoops3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i < 0; i < 10; i += 1 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ForLoops4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := 0; i := 10; i += 1 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ForLoops5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := 0; i < 10; i < 1 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ForLoops6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i < 0; ; i += 1 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ForLoops7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := true; i != false; i = !i {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ForLoops8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := true; i := 2; i = !i {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ForLoops9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for ; i != false; i = !i {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ForLoops10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := true; i != false; {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ForLoops11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := true; i != false; {\n"
        "        for i := true; i != false; {\n"
        "            for i := true; i != false; {\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ForLoops12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := true; i != false; {\n"
        "        for a := 4; a < 5; ; a += 3 {\n"
        "        }\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Test expressions in if-else if-else statements ===

TEST_F(ParserScannerTest, ExpressionInIfStatements1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        foo = 42\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a != 1 {\n"
        "        foo = 42\n"
        "    } else if \n!!true {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}


TEST_F(ParserScannerTest, ExpressionInIfStatements3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a := 3 {\n"
        "        foo = 42\n"
        "    } else if b {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if true {\n"
        "        foo = 42\n"
        "    } else if \n false {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a == a {\n"
        "        foo = 42\n"
        "    } else if \n b != b {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if (a - 8) * 7 == b + 8 / 7 {\n"
        "        foo = 42\n"
        "    } else if \n !b && (c || d) != b || !(t && r) {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
         "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else \n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements14) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 \n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements15) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo = 1\n"
        "        } else {\n"
        "            if a < 4 {\n"
        "                foo = 42\n"
        "            } else if b {\n"
        "                foo = 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        }\n"
        "    } else if b {\n"
        "        foo = 1\n"
        "    } else {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements16) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if\n a + 4 - 8 / 7 <= 745 {\n"
        "        foo = 42\n"
        "    } else if \n true && !a || b || false {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}
