/** @file parser_scanner.cpp
 *
 * IFJ20 compiler tests
 *
 * @brief Contains tests covering mainly the parser functionality, though in cooperation with scanner.
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
#include "stdin_mock_test.h"

extern "C" {
#include "scanner.h"
#include "parser.h"
#include "control_flow.h"
#include "ast.h"
#include "code_generator.h"
}


class ParserScannerTest : public StdinMockingScannerTest {
protected:
    void ComplexTest(std::string &inputStr, CompilerResult expected_compiler_result);
    void ComplexTest(std::string &inputStr, CompilerResult expected_compiler_result, bool enable_ast_checks);

    void CheckStatementRecursively(CFStatement *st);

    void SetUp() override {
        StdinMockingScannerTest::SetUp();
        cf_error = CF_NO_ERROR;
        ast_set_strict_inference_state(false);
    }
};

void ParserScannerTest::CheckStatementRecursively(CFStatement *st) {
    bool x;
    static int a = 0;
    switch (st->statementType) {
        case CF_RETURN:
        case CF_BASIC:
            if (st->data.bodyAst == NULL) {
                if (st->followingStatement != NULL) {
                    CheckStatementRecursively(st->followingStatement);
                }
                return;
            }

            EXPECT_TRUE(ast_infer_node_type(st->data.bodyAst));
            return;
        case CF_IF:
            EXPECT_TRUE(ast_infer_node_type(st->data.ifData->conditionalAst));

            if (st->data.ifData->thenStatement != NULL)
                CheckStatementRecursively(st->data.ifData->thenStatement);
            if (st->data.ifData->elseStatement != NULL)
                CheckStatementRecursively(st->data.ifData->elseStatement);

            return;
        case CF_FOR:
            if (st->data.forData->conditionalAst != NULL)
                EXPECT_TRUE(ast_infer_node_type(st->data.forData->conditionalAst));
            if (st->data.forData->afterthoughtAst != NULL)
                EXPECT_TRUE(ast_infer_node_type(st->data.forData->afterthoughtAst));
            if (st->data.forData->definitionAst != NULL)
                EXPECT_TRUE(ast_infer_node_type(st->data.forData->definitionAst));

            CheckStatementRecursively(st->data.forData->bodyStatement);
            return;
    }
}

void ParserScannerTest::ComplexTest(std::string &inputStr, CompilerResult expected_compiler_result) {
    ComplexTest(inputStr, expected_compiler_result, true);
}

void ParserScannerTest::ComplexTest(std::string &inputStr, CompilerResult expected_compiler_result, bool enable_ast_checks) {
#if VERBOSE
    std::cout << "[TEST] Input string:\n" << inputStr << "\n[START]\n";
#endif
    buffer->sputn(inputStr.c_str(), inputStr.length());
    buffer->sputc(EOF);

    CompilerResult parserRes = parser_parse();
    std::cout << "[RESULT] Compiler result: " << resultNames.find(parserRes)->second << '\n';

    if (compiler_result == COMPILER_RESULT_SUCCESS) {
        if(enable_ast_checks) {
            ast_set_strict_inference_state(true);
            CFProgram *prog = get_program();
            ASSERT_NE(prog, nullptr);

            CFFuncListNode *fn = prog->functionList;
            while (fn != NULL) {
                CFFunction *f = &fn->fun;
                ASSERT_NE(f, nullptr);

                CFStatement *st = f->rootStatement;

                while (st != NULL) {
                    CheckStatementRecursively(st);
                    st = st->followingStatement;
                }

                fn = fn->next;
            }
        }

        if (compiler_result == COMPILER_RESULT_SUCCESS) {
            tcg_generate();
        }
    }

    std::cout << "[RESULT] Expected: " << resultNames.find(expected_compiler_result)->second << ", Actual: "
              << resultNames.find(compiler_result)->second << '\n';
    ASSERT_EQ(compiler_result, expected_compiler_result);
}

TEST_F(ParserScannerTest, BasicCode) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 0 * 8\n"
        "    a -= 5 - 7\n"
        "\n"
        "    if 0 < 1 {\n"
        "        print(\"Will always be printed.\\n\")\n"
        "    } else {\n"
        "        print(\"Will never be seen.\\n\")\n"
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
        "    abc := 672\n"
        "    a := 666 - abc\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, MinimalisticIfFunction) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 1\n"
        "    if a := 1 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, RedefinitionInNestedFrame) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 1\n"
        "    if a == 1 {\n"
        "        a := true\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, RedefinitionInFrame) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 1\n"
        "    if a == 1 {\n"
        "    }\n"
        "    a := true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, PackageTypo) {
    std::string inputStr = \
        "pckage main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    if a == true {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, MissingPackageMain) {
    std::string inputStr = \
        "package foo\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    if a == true {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingMainFunc) {
    std::string inputStr = \
        "package main"
        "func main() {\n"
        "    a := false\n"
        "    if a == true {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError1) {
    std::string inputStr = \
        "package main "
        "func main() {\n"
        "    a := false\n"
        "    if a == true {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {"
        "    a := false\n"
        "    if a == true {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    if a == true {"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    if a == true {"
        "    }"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLMissingError5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    if a == true {\n"
        "        if a != true {"
        "           a = 5\n"
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
        "    a := false\n"
        "    if a == true {\n"
        "        if a != true {\n"
        "           a = false"
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
        "    if 1-2+3 == 0 {\n"
        "        a := false\n"
        "        abc := true\n"
        "        if !a == abc {\n"
        "            b := \"String\\n\"\n"
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
        "    if 1-2+3 == 0 {\n"
        "        if 1*2/3 == 0 {\n"
        "            a := \"String\\n\"\n"
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
        "    if 1-2+3 == 0 {\n"
        "        if 1*2/3 == 0 {\n"
        "            a := \"String\\n\"\n"
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
        "    for a := 5; a < 79; a = a + 8 {"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoop2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; a < 79; a = a + 8 {\n"
        "    }"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoop3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; \n a < 79; a = a + 8 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for \n a := 5; a < 79; a = a + 8 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a\n := 5; a < 79; a = a + 8 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; a < 79\n; a = a + 8 { {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; a < 79; \na = a + 8 { {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; a < 79; a = a + 8\n {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for a := 5; a < 79; a = a + 8 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInForLoopForbidden7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 0\n"
        "    for \n ; a < 79; a = a + 8 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
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

// === Test EOLs in assing statement ===

TEST_F(ParserScannerTest, AssignEOLMissing) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b := 1 + two, 2 / three"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOLForbidden1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a\n, b := 1 + two, 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOL2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a,\n b := 1 + two, 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AssignEOL3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b\n := 1 + two, 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOL4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b := \n1 + two, 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AssignEOL5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b := 1\n + two, 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, AssignEOL6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b := 1 + two,\n 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AssignEOL7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b := 1 + two\n , 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, AssignEOL8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b := 1 + two, 2\n / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignEOL9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b := 1 + two, 2 / \n three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AssignEOL10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "two := 2\n"
        "three := 3\n"
        "    a, b := 1 + \n two, 2 /\n three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AssignStatement1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 36478\n"
        "    b := a = 4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignStatement2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 36478 ** 1\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignStatement3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "     := 36478 * 1\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignStatement4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignStatement5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := 1\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, AssignStatement6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := 1, 2, 3\n"

        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, AssignStatement7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    ab := 1, 2\n"

        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, AssignStatement8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := foo\n"
        "}\n"
        "func foo() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

// === Test EOL in commands ===

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    two := 2\n"
        "    a := 3\n"
        "    a\n = 1 + two\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    two := 2\n"
        "    a := 3\n"
        "    b := 3\n"
        "    a, b =\n 1 + \n two,\n 2 /\n two\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    two := 3\n"
        "    three := 2\n"
        "    a := 4\n"
        "    b := 2\n"
        "    a, b \n = 1 + two, 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    two := 3\n"
        "    three := 2\n"
        "    a := 4\n"
        "    b := 2\n"
        "    a, b = 1\n+ two, 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    two := 3\n"
        "    three := 2\n"
        "    a := 4\n"
        "    b := 2\n"
        "    a, b =\n 1 +\n two,\n 2 /\n three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    two := 3\n"
        "    three := 2\n"
        "    a := 4\n"
        "    a, _ = 1 + two, 2 / three\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInDefinitionOfVariable7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    two := 3\n"
        "    three := 2\n"
        "    a := 4\n"
        "    _\n, a := 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test EOL in if-else statements ===

TEST_F(ParserScannerTest, EOLInIfElseStatement1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if \na < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar \n{\n"
        "        foo := 42\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    \n\n\n\n"
        "    } else {\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "    \n\n\n\n"
        "        foo := 42\n"
        "    } else {\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {"
        "        foo := 42\n"
        "    } else {\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else {"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if a < b || b != foobarvar {\n"
        "        foo := 1\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if \na < b || b != foobarvar {\n"
        "        foo := 1\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if a < b || b != foobarvar\n {\n"
        "        foo := 1\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } \n else if a < b || b != foobarvar {\n"
        "        foo := 1\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if a < b || b != foobarvar {"
        "        foo := 1\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if {\n"
        "        foo := 1\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if a < b || b != foobarvar \n {\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if \n a < b || b != foobarvar {\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if\n a < 0 && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if \n a < b || b != foobarvar {\n"
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
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if a < 0 && b >= foobarvar \n{\n"
        "        foo := 42\n"
        "    } else if \n a < b || b != foobarvar {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if\n a <\n 0 &&\n b >=\n foobarvar {\n"
        "        foo := 42\n"
        "    } else if \n a < b || b != foobarvar {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if\n a < 0\n && b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if \n a < b || b != foobarvar {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if\n a < 0 && b \n>= foobarvar {\n"
        "        foo := 42\n"
        "    } else if \n a < b || b != foobarvar {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if\n a < 0 && b >\n= foobarvar {\n"
        "        foo := 42\n"
        "    } else if \n a < b || b != foobarvar {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInIfElseIfStatement9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4\n"
        "    b := 5\n"
        "    foobarvar := -1\n"
        "    if\n a < 0 &\n& b >= foobarvar {\n"
        "        foo := 42\n"
        "    } else if \n a < b || b != foobarvar {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_LEXICAL);
}


// === Test EOL in function call ===

TEST_F(ParserScannerTest, EOLInFunctionCall1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7 + (-9)) < 0) && !true , var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo\n(((a + 7 + (-9)) < 0) && !true , var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7 + (-9)) < 0) && !true \n, var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(\n(\n(\na + 7 + (\n-9)) < 0) && !true , var * 8 - (\nvar2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}


TEST_F(ParserScannerTest, EOLInFunctionCall5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7 + (-9)) < 0) && !true ,\n var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7 + (-9)) < 0) && !true , var * 8 - (var2)\n)\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(a int) < 0) && !true , var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := \"test string\"\n"
        "    foo((a < 0) && !true , var2 string)\n"
        "}\n"
        "\n"
        "func foo(boolean bool, str string) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := \"test string\"\n"
        "    foo((a int < 0) && !true , var2 string)\n"
        "}\n"
        "\n"
        "func foo(boolean bool, str string) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}


TEST_F(ParserScannerTest, EOLInFunctionCall10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7 + (-9)) <\n 0) &&\n !true ,\n var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a +\n 7 +\n (-9)) <\n 0) &&\n !true ,\n var *\n 8 -\n (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a\n + 7 + (-9)) <\n 0) &&\n !true ,\n var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7 + (\n-9)) <\n 0) &&\n !true ,\n var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall14) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7\n + (-9)) < 0) && !true , var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall15) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7\n + (-9))\n < 0) && !true , var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall16) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7 + (-\n9)) < 0) && !\ntrue , var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall17) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7\n + (-9)) < 0) && !true , var\n * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall18) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + 7\n + (-9)) < 0) && !true , var * 8 - (var2\n))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall19) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        " foo(((a + bar(7) + (-9)) < 0) && !true , var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n"
        "func bar(integer int) int {\n"
        " return 1\n"
        "}\n";
    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall20) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        " a := 6\n"
        " var := 6\n"
        " var2 := 6\n"
        "    foo(((a + bar(7, bar(54 / 6, 6-9)) + (-9)) < 0) && !true , var * 8 - (var2))\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n"
        "func bar(integer int, integer2 int) (i int) {\n"
        "    return integer + integer2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall21) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    foo(\n(a))\n"
        "}\n"
        "\n"
        "func foo(boolean bool) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall22) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    foo((a)\n)\n"
        "}\n"
        "\n"
        "func foo(boolean bool) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, EOLInFunctionCall23) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    foo((a))\n"
        "}\n"
        "\n"
        "func foo(boolean bool) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall24) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    b := 5\n"
        "    foo((a), b)\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, EOLInFunctionCall25) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    b := 5\n"
        "    foo((a), b int)\n"
        "}\n"
        "\n"
        "func foo(boolean bool, integer int) {\n"
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
        "func main() {\n"
        "    _ = foobar()\n"
        "}\n"
        "func foobar() bool {\n"
        "    a := false\n"
        "    return a && foo((a))\n"
        "}\n"
        "\n"
        "func foo(boolean bool) bool {\n"
        "    return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _ = foobar()\n"
        "}\n"
        "func foobar() bool {\n"
        "    a := false\n"
        "    return \na && foo((a))\n"
        "}\n"
        "\n"
        "func foo(boolean bool) bool {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _, _ = foobar()\n"
        "}\n"
        "func foobar() (int, int, int) {\n"
        "    a, b, g, c := 2, 4, 78, 878\n"
        "    return a + foo(a), b / bar(b, foo(g + 5 * c)), bar(b, c)\n"
        "}\n"
        "func foo(i int) int {\n"
        "    return i\n"
        "}\n"
        "func bar(i int, i2 int) int {\n"
        "    return i\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _, _ = foobar()\n"
        "}\n"
        "func foobar() (int, bool, float64) {\n"
        "    a, b, g, c := 2, false, 78, 878\n"
        "    return a + foo(a), b || bar(b, foo(g + 5 * c)) && true, 7.4/8.7*9.0+8.0+5982.4\n"
        "}\n"
        "\n"
        "func foo(integer int) int {\n"
        "    return integer + 5\n"
        "}\n"
        "func bar(boolean bool, integer int) bool {\n"
        "    if boolean {\n"
        "        return true\n"
        "    } else {\n"
        "        return false\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _ = foobar()\n"
        "}\n"
        "func foobar() (int, string, float64) {\n"
        "    a, b, g, c := 2, false, 78, 878\n"
        "    return a + foo(a), b || bar(b, foo(g + 5 * c)) && true\n, 7.4/8.7*9.0+8.0+5982.4\n"
        "}\n"
        "\n"
        "func foo(integer int) int {\n"
        "    return integer + 5\n"
        "}\n"
        "func bar(boolean bool, integer int) bool {\n"
        "    if boolean {\n"
        "        return true\n"
        "    } else {\n"
        "        return false\n"
        "    }\n"
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
        "    return 5, \"returned \\n \\xaf string\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _, _ = foobar()\n"
        "}\n"
        "func foobar() (int, bool, float64) {\n"
        "    a, b, g, c := 2, false, 78, 878\n"
        "    return a +\n foo(\na),\n b ||\n bar(\nb,\n foo(\ng +\n5 *\n c)) &&\n true,\n 7.4/\n8.7*\n9.0+\n8.0+\n5982.4\n"
        "}\n"
        "\n"
        "func foo(integer int) int {\n"
        "    return integer + 5\n"
        "}\n"
        "func bar(boolean bool, integer int) bool {\n"
        "    if boolean {\n"
        "        return true\n"
        "    } else {\n"
        "        return false\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat14) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _, _ = foobar()\n"
        "}\n"
        "func foobar() (int, bool, float64) {\n"
        "    a, b, g, c := 2, false, 78, 878\n"
        "    return (a + foo(a)), (b || bar(b, foo(g + 5 * c)) && true), (7.4/8.7*9.0+8.0+5982.4)\n"
        "}\n"
        "\n"
        "func foo(integer int) int {\n"
        "    return integer + 5\n"
        "}\n"
        "func bar(boolean bool, integer int) bool {\n"
        "    if boolean {\n"
        "        return true\n"
        "    } else {\n"
        "        return false\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ReturnFormat15) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _, _ = foobar()\n"
        "}\n"
        "func foobar() (int, string, float64) {\n"
        "    a, b, g, c := 2, false, 78, 878\n"
        "    return (a + foo(a), b || bar(b, foo(g + 5 * c)) && true, 7.4/8.7*9.0+8.0+5982.4)\n"
        "}\n"
        "\n"
        "func foo(integer int) int {\n"
        "    return integer + 5\n"
        "}\n"
        "func bar(boolean bool, integer int) bool {\n"
        "    if boolean {\n"
        "        return true\n"
        "    } else {\n"
        "        return false\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat16) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _, _ = foobar()\n"
        "}\n"
        "func foobar() (int, string, float64) {\n"
        "    a, b, g, c := 2, false, 78, 878\n"
        "    return a + foo(a)\n, b || bar(b, foo(g + 5 * c)) && true, 7.4/8.7*9.0+8.0+5982.4\n"
        "}\n"
        "\n"
        "func foo(integer int) int {\n"
        "    return integer + 5\n"
        "}\n"
        "func bar(boolean bool, integer int) bool {\n"
        "    if boolean {\n"
        "        return true\n"
        "    } else {\n"
        "        return false\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ReturnFormat17) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _, _ = foobar()\n"
        "}\n"
        "func foobar() (int, bool, float64) {\n"
        "    a, b, g, c := 2, false, 78, 878\n"
        "    return a + foo(a), b || bar(b, foo\n(g + 5 * c)) && true, 7.4/8.7*9.0+8.0+5982.4\n"
        "}\n"
        "\n"
        "func foo(integer int) int {\n"
        "    return integer + 5\n"
        "}\n"
        "func bar(boolean bool, integer int) bool {\n"
        "    if boolean {\n"
        "        return true\n"
        "    } else {\n"
        "        return false\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, ReturnFormat18) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _ = foobar()\n"
        "}\n"
        "func foobar() int {\n"
        "    a := false\n"
        "    return a + foo((a))\n"
        "}\n"
        "\n"
        "func foo(boolean bool) int {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, ReturnFormat19) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _ = foobar()\n"
        "}\n"
        "func foobar() int {\n"
        "    a := false\n"
        "    return a && foo((a))\n"
        "}\n"
        "\n"
        "func foo(boolean bool) int {\n"
        "    return 1\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION, false);
}

TEST_F(ParserScannerTest, ReturnFormat20) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foobar()\n"
        "}\n"
        "func foobar() {\n"
        "    return false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ReturnFormat21) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _ = foobar()\n"
        "}\n"
        "func foobar() (bool, int) {\n"
        "    return false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ReturnFormat22) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _ = foobar()\n"
        "}\n"
        "func foobar() bool {\n"
        "    return false, 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ReturnFormat23) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _ = foobar()\n"
        "}\n"
        "func foobar() bool {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ReturnFormat24) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _, _ = foobar()\n"
        "}\n"
        "func foobar() (bool, int, string) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

// === Insert more whitespaces ===

TEST_F(ParserScannerTest, InsertMoreWhitespaces1) {
    std::string inputStr = \
        "package     main\n"
        "  \n"
        "func main(   )    {\n"
        "    a, b := 0, 0\n"
        "    a    , b        = 1,     2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, InsertMoreTabs) {
    std::string inputStr = \
        "package\tmain\n"
        "\t\n"
        "func main(\t) {\n"
        "    a, b := 0, 0\n"
        "    a\t, b = 1,\t\t\t2\n"
        "\t\t\t}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments1) {
    std::string inputStr = \
        "package main\n"
        "\n /* comment to be skipped */"
        "func main() {\n"
        "    a, b := 0, 0\n"
        "    a, b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() { // one-line comment \n"
        "    a, b := 0, 0\n"
        "    a, b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() { /* multi-line comment \n \n about stuff */ \n"
        "    a, b := 0, 0\n"
        "    a, b = 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := 0, 0\n"
        "    a, b = /* little in-command comment */ 1, 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AddComments5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 5/* Nested /*comment */ 7*/ + 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
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
        "    return 5, \"string text\", 3.1415\n"
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
        "    return 5, \"string text\", 3.1415\n"
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
        "    return 5\n"
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
        "    return 5, \"string text\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionDefinition9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i int, s string, f float64) (int, string, float64) {\n"
        "    return 5, \"string text, \"3.1415\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionDefinition10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i int, s string, f float64) (int, float64) {\n"
        "    return 5, \"string text\", 3.1415\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, FunctionDefinition11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i int, s string, f float64) (int, string, ) {\n"
        "    return 5, \"string text\", 3.1415\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionDefinition12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i int, s string, f float64) (int, string, float64) {\n"
        "    return 5, \"string text\" 3.1415\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionDefinition13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "\n"
        "func foo(i int, s string, f float64) (int, string, float64) {\n"
        "    func bar() {\n"
        "    }\n"
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
        "func foo() (i int, s string) {\n"
        "    a := false"
        "    b := 4\n"
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
        "    a, _ = inputi()\n"
        "    if a < 0 {\n"
        "        print(\"Faktorial nejde spocitat!\\n\")\n"
        "    } else {\n"
        "        vysl := 1\n"
        "        for ; a > 0; a = a - 1 {\n"
        "            vysl = vysl * a\n"
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
        "    dec_n := n - 1\n"
        "    if n < 2 {\n"
        "        return 1\n"
        "    } else {\n"
        "        tmp := 0\n"
        "        tmp = factorial(dec_n)\n"
        "        return n * tmp\n"
        "    }\n"
        "}\n"
        "func main() {\n"
        "    print(\"Zadejte cislo pro vypocet faktorialu: \")\n"
        "    a := 0\n"
        "    err := 0\n"
        "    a, err = inputi()\n"
        "    if err == 0 {\n"
        "        if a < 0 {\n"
        "            print(\"Faktorial nejde spocitat!\", \"\\n\")\n"
        "        } else {\n"
        "            vysl := 0\n"
        "            vysl = factorial(a)\n"
        "            print(\"Vysledek je \", vysl, \"\\n\")\n"
        "        }\n"
        "    } else {\n"
        "        print(\"Chyba pri nacitani celeho cisla!\\n\")\n"
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
        "    s2 := s1 + \", ktery jeste trochu obohatime\"\n"
        "    print(s1, \"\\n\", s2)\n"
        "    s1len := 0\n"
        "    s1len = len(s1)\n"
        "    s1len = s1len - 4\n"
        "    s1, _ = substr(s2, s1len, 4)\n"
        "    s1len = s1len + 1\n"
        "    print(\"4 znaky od\", s1len, \". znaku v \\\"\", s2, \"\\\":\", s1, \"\\n\")\n"
        "    print(\"Zadejte serazenou posloupnost vsech malych pismen a-h, \")\n"
        "    print(\"pricemz se pismena nesmeji v posloupnosti opakovat: \")\n"
        "    err := 0\n"
        "    s1, err = inputs()\n"
        "    if err != 1 {\n"
        "        for ;s1 != \"abcdefgh\"; {\n"
        "            print(\"\\n\", \"Spatne zadana posloupnost, zkuste znovu: \")\n"
        "            s1, _ = inputs()\n"
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
        "    LongIndetifier_djlajdlawjdlajwdjawdklawjdlawjkldawjdjawdjawkljdklawjdkljawkldjawdjawdjawkldjawdlajdawkldawkldjawdlawkldawjdawjdawjdklawjdaw := 1\n"
        "    a := LongIndetifier_djlajdlawjdlajwdjawdklawjdlawjkldawjdjawdjawkljdklawjdkljawkldjawdjawdjawkldjawdlajdawkldawkldjawdlawkldawjdawjdawjdklawjdaw\n"
        "    LongIndetifier_ahdwjlahwdjawhldahwdawhdawlhdawlkhdklawhdioawiodhaiowdhahdwhawiodhiawhdhawidhawiodhahwdiohawiodhiawdhioawhdawhiodawhidhawdohawihdiawdawd := \"test\"\n"
        "return a, LongIndetifier_ahdwjlahwdjawhldahwdawhdawlhdawlkhdklawhdioawiodhaiowdhahdwhawiodhiawhdhawidhawiodhahwdiohawiodhiawdhioawhdawhiodawhidhawdohawihdiawdawd\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Others ===

TEST_F(ParserScannerTest, StringConcatenation1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"te\" + \"st\" + \"str\" + \"ing\"\n"
        "}\n";
    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, StringConcatenation2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"te\" + \"st\" \"str\" + \"ing\"\n"
        "}\n";
    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

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
        "}\n"
        "func foo() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, MissingMainFunc1) {
    std::string inputStr = \
        "package main\n"
        "\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, MissingMainFunc2) {
    std::string inputStr = \
        "package main\n"
        "func foo() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, AssignToVoidVariable1) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "   _ = foo()\n"
        "}\n"
        "func foo() bool {\n"
        "    return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, AssignToVoidVariable2) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "   _, _ = foo()\n"
        "}\n"
        "func foo() (bool, bool, bool) {\n"
        "    return true, true, false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL, false); //TODO which error code should be returned?
}

TEST_F(ParserScannerTest, AssignToVoidVariable3) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "   _, _, _ = foo()\n"
        "}\n"
        "func foo() (bool, bool, bool) {\n"
        "    return true, true, false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, DefineToVoidVariable1) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "   _ := true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE); //TODO which error code should be returned?
}

TEST_F(ParserScannerTest, DefineToVoidVariable2) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "   _, _, _ := true, true, true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, VoidVariableInExpression) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "    a := _ + 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, TextAtTheEndOfFile1) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "    a, b := 0, 5\n"
        "}\n"
        "bla bla bla\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, TextAtTheEndOfFile2) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "    a, b := 0, 5\n"
        "}\n"
        "/*comment*/\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, TextAtTheEndOfFile3) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "    a, b := 0, 5\n"
        "}\n"
        "// comment \n";

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
        "   return false\n"
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
        "    return false, i\n"
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
        "    return false, 9\n"
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
        "return true\n"
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
        "    return true, 0\n"
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
        "    return true, 0\n"
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
        "    return true, 0\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo foo(i int, s string) (bool, int) {\n"
        "    return true, 0\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo (i int, s string) () {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionHeader11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo (i int, s string) () {\n"
        "    {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo (i int, s string) () {\n"
        "    func bar (i int, s string) () {\n"
        "    }\n";
    "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo (54 int, s string) () {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader14) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "func foo (i int, s string) (foo) {\n"
        "    return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader15) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "}\n"
        "a := 5\n"
        "func foo (i int, s string) () {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionHeader16) {
    std::string inputStr = \
        "package main\n"
        "\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
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

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_LEXICAL);
}

TEST_F(ParserScannerTest, LexicalErrorCompilerResult4) {
    std::string inputStr = \
        "pa]ckage main\n"
        "\n"
        "func main() {\n"
        "    a := \"foo\\sbar\" \n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, LexicalErrorCompilerResult5) {
    std::string inputStr = \
        "]package main\n"
        "\n"
        "func main() {\n"
        "    a := \"foo\\sbar\" \n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_LEXICAL);
}

// === Test function call ===

TEST_F(ParserScannerTest, FunctionCall1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 1\n"
        "    b := true\n"
        "    foo(a, b)\n"
        "}\n"
        "func foo(i int, boolean bool) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(5+4)\n"
        "}\n"
        "func foo(i int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(true)\n"
        "}\n"
        "func foo(boolean bool) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"string\"\n"
        "    a = foo()\n"
        "}\n"
        "func foo() string {\n"
        "    return \"returned string\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := foo()\n"
        "}\n"
        "func foo() (int) {\n"
        "    return 789\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := true\n"
        "    a = foo(4, 5)\n"
        "}\n"
        "func foo(i1 int, i2 int) (boolean bool) {\n"
        "    return false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := foo(true, 7, 5.4)\n"
        "}\n"
        "func foo(bool1 bool, i1 int, f1 float64) (boolean bool) {\n"
        "    return false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := foo()\n"
        "}\n"
        "func foo() (boolean bool, integer int) {\n"
        "    return false, 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := foo(7, 4, 1)\n"
        "}\n"
        "func foo(i1 int, i2 int, i3 int) (int, int) {\n"
        "    return i1, i2 + i3\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, _ := foo(7 + 4, 3 + 1)\n"
        "}\n"
        "func foo(i1 int, i2 int) (int, int) {\n"
        "    return i1, i2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    _, _ := foo()\n"
        "}\n"
        "func foo() (int, int) {\n"
        "    return 1, 1 + 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, FunctionCall12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    b := false\n"
        "    a, _, _ := foo(true && b, 4 + 5 / 7)\n"
        "}\n"
        "func foo(boolean bool, i int) (int, int, string) {\n"
        "    return 0, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    b := 3\n"
        "    a, _, _ := foo(7 + 5 < 3 && 4 + b > 2)\n"
        "}\n"
        "func foo(boolean bool) (int, int, string) {\n"
        "    return 0, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall14) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "   a, b := 0, 1\n"
        "    a, _, _ = foo((a * 7 - 4) != 0 && (b - 9 * (-8)) == a, \"test\")\n"
        "}\n"
        "func foo(boolean bool, str string) (int, int, string) {\n"
        "    return 0, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall15) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := true\n"
        "    a, _, _ = foo((a))\n"
        "}\n"
        "func foo(boolean bool) (bool, int, string) {\n"
        "    return true, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall16) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := true, false\n"
        "    a, _, _ = foo((a && b))\n"
        "}\n"
        "func foo(boolean bool) (bool, int, string) {\n"
        "    return true, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionCall17) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := true, false\n"
        "    a, _, _ = 5 + foo(a && b)\n"
        "}\n"
        "func foo(boolean bool) (int, int, string) {\n"
        "    return 0, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, FunctionCall18) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := true, false\n"
        "    a, _, _ = 5 / foo(a && b)\n"
        "}\n"
        "func foo(boolean bool) (int, int, string) {\n"
        "    return 0, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, FunctionCall19) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := true, false\n"
        "    a, _, _ = !foo(a && b)\n"
        "}\n"
        "func foo(boolean bool) (bool, int, string) {\n"
        "    return boolean, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, FunctionCall20) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := true, false\n"
        "    a, _, _ = +5 - (-5) / !foo(a && b)\n"
        "}\n"
        "func foo(boolean bool) (bool, int, string) {\n"
        "    return boolean, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, FunctionCall21) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := +5 - (-5) / !foo(true && false)\n"
        "}\n"
        "func foo(boolean bool) (bool) {\n"
        "    return boolean\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, FunctionCall22) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    !foo(a && b)\n"
        "}\n"
        "func foo(boolean bool) (bool, int, string) {\n"
        "    return boolean, 1 + 2, \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionCall23) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    1 + foo(a && b)\n"
        "}\n"
        "func foo(boolean bool) (int) {\n"
        "    return 1 + 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionCall24) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := false, false\n"
        "    foo(a && b) - 1\n"
        "}\n"
        "func foo(boolean bool) (int) {\n"
        "    return 1 + 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionCall25) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := false, false\n"
        "    foo(a && b) - 8 / 7\n"
        "}\n"
        "func foo(boolean bool) (int) {\n"
        "    return 1 + 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, FunctionCall26) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(true)\n"
        "}\n"
        "func foo(boolean bool) (int) {\n"
        "    return 1 + 2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SEMANTIC_GENERAL);
}

TEST_F(ParserScannerTest, FunctionCall27) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(true)\n"
        "}\n"
        "func foo(boolean bool) {\n"
        "    return \n"
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
        " a := \n 3\n"
        "}\n"
        "func foo() {\n"
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
        "    for i := true; j := 2; i = !i {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, ForLoops9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    i := true\n"
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
        "        for a := 4; a < 5; a += 3 {\n"
        "        }\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ForLoops13) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := true; 5 - 8; {\n"
        "        for a := 4; a < 5; a += 3 {\n"
        "        }\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, ForLoops14) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := true; \"\"; {\n"
        "        for a := 4; a < 5; a += 3 {\n"
        "        }\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, ForLoops15) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    j := 0\n"
        "    for i := 0; j := 10; i += 1 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

// === Test expressions in if-else if-else statements ===

TEST_F(ParserScannerTest, ExpressionInIfStatements1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a, b := 0, true\n"
        "    if a < 4 {\n"
        "        foo := 42\n"
        "    } else if b {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a != 1 {\n"
        "        foo := 42\n"
        "    } else if \n!!true {\n"
        "        foo := 1\n"
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
        "    a := 1\n"
        "    if a = 3 {\n"
        "        foo := 42\n"
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
        "        foo := 42\n"
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
        "    a, b := 0, true\n"
        "    if a == a {\n"
        "        foo := 42\n"
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
        "    a, b, c, d, t, r := 0, 7, true, false, true, true\n"
        "    if (a - 8) * 7 == b + 8 / 7 {\n"
        "        foo := 42\n"
        "    } else if \n !d && (c || d) != false || !(t && r) {\n"
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
        "    a, b, c, d := 0, true, false, true\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            } else if b && c || d {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if 6 * 7 == 42 {\n"
        "            foo := 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if true || false {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            } else if b {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "        } else if b {\n"
        "            foo := 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo := 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            } else if b {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo := 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            } else if b {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            } else if b {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo := 1\n"
        "        } else \n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if {\n"
        "                foo := 42\n"
        "            } else if b {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo := 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a < 4 \n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            } else if b {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo := 1\n"
        "        } else {\n"
        "            bar := 1\n"
        "        }\n"
        "    } else if b {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if a < 4 {\n"
        "        if a < 4 {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            } else if b {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        } else if b {\n"
        "            foo := 1\n"
        "        } else {\n"
        "            if a < 4 {\n"
        "                foo := 42\n"
        "            } else if b {\n"
        "                foo := 1\n"
        "            } else {\n"
        "                bar := 1\n"
        "            }\n"
        "        }\n"
        "    } else if b {\n"
        "        foo := 1\n"
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
        "    a, b := 0, true\n"
        "    if\n a + 4 - 8 / 7 <= 745 {\n"
        "        foo := 42\n"
        "    } else if \n true && !(a > 3) || b || false {\n"
        "        bar := 1\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements17) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if ((0==251) {\n"
        "    } else {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, ExpressionInIfStatements18) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if 0==0 {\n"
        "    } else {\n"
        "    }\n"
        "    for ;0!=0; {\n"
        "    }\n"
        "    if 0<1 {\n"
        "    } else {\n"
        "    }\n"
        "    if 0>1 {\n"
        "    } else {\n"
        "    }\n"
        "    for ;1<=0; {\n"
        "    }\n"
        "    for ;0>=1; {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

// === Test redefinition of variable ===

TEST_F(ParserScannerTest, RedefinitionOfVariable1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4\n"
        "    a := 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, RedefinitionOfVariable2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 5.2\n"
        "    a := 4.2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, RedefinitionOfVariable3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"Test string\\n\"\n"
        "    a := \"Another test string\\n\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, RedefinitionOfVariable4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := true\n"
        "    a := false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

// === Try to change data type of a variable ===

TEST_F(ParserScannerTest, ChangeDataType1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := false\n"
        "    a = 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, ChangeDataType2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 5\n"
        "    a = 4.2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, ChangeDataType3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"test string\"\n"
        "    a = 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, ChangeDataType4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4.2\n"
        "    a = 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

// === Undefined variable ===

TEST_F(ParserScannerTest, UndefinedVariable1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a = 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, UndefinedVariable2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a = 4.2\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, UndefinedVariable3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a = \"test string\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, UndefinedVariable4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a = false\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

// === Undefined function ===

TEST_F(ParserScannerTest, UndefinedFunction1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo()\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, UndefinedFunction2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, 7, true)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, UndefinedFunction3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := foo()\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, UndefinedFunction4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4 / foo()\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, UndefinedFunction5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    bar(4, foo())\n"
        "}\n"
        "func bar(i int, b bool) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

// === Redefined function ===

TEST_F(ParserScannerTest, RedefinedFunction1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo()\n"
        "}\n"
        "func foo() {\n"
        "}\n"
        "func foo() {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, RedefinedFunction2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, 7, true)\n"
        "}\n"
        "func foo(i int, i2 int, b bool) {\n"
        "}\n"
        "func foo(b bool, f float64, s string) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, RedefinedFunction3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := foo()\n"
        "}\n"
        "func foo() int {\n"
        "    return 8\n"
        "}\n"
        "func foo() string {\n"
        "    return \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, RedefinedFunction4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4 / foo()\n"
        "}\n"
        "func foo() int {\n"
        "    return 8\n"
        "}\n"
        "func foo() float64 {\n"
        "   return 5.8\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

TEST_F(ParserScannerTest, RedefinedFunction5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    bar(4, foo())\n"
        "}\n"
        "func bar(i int, b bool) {\n"
        "}\n"
        "func foo() bool {\n"
        "   return false\n"
        "}\n"
        "func foo() int {\n"
        "    return 5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

// === Data type compatibility ===

TEST_F(ParserScannerTest, DataTypeCompatibility1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 5 + foo()\n"
        "}\n"
        "func foo() int {\n"
        "    return 3\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, DataTypeCompatibility2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, 7, true)\n"
        "}\n"
        "func foo(f float64, i2 int, b bool) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, DataTypeCompatibility3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := foo() + 5.7\n"
        "}\n"
        "func foo() int {\n"
        "    return 8\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION, false);
}

TEST_F(ParserScannerTest, DataTypeCompatibility4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4 / foo(5.6)\n"
        "}\n"
        "func foo(i int) int {\n"
        "    return 8\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, DataTypeCompatibility5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    bar(4, foo())\n"
        "}\n"
        "func bar(i int, b bool) {\n"
        "}\n"
        "func foo() string {\n"
        "   return \"str\\n\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE, false);
}

// === Data type compatibility in expression ===

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 5 - 5.5\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    bar(4 || true, foo())\n"
        "}\n"
        "func bar(i int, b bool) {\n"
        "}\n"
        "func foo() bool {\n"
        "   return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    bar(4, true && foo())\n"
        "}\n"
        "func bar(i int, b bool) {\n"
        "}\n"
        "func foo() string {\n"
        "   return \"str\\n\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION, false);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"str1 \" + \"str2\\n\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"str1 \" - \"str2\\n\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4.6 + \"str2\\n\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := \"str1 \" + true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 3 / float64\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_SYNTAX_OR_WRONG_EOL);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 3 / 6.4\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 3 / float2int(6.4)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    if 5.4 > 2 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION);
}

TEST_F(ParserScannerTest, DataTypeCompatibilityInExpression12) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    for i := 0; i < 10; i += 1.2 {\n"
        "    }\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_TYPE_INCOMPATIBILITY_IN_EXPRESSION, false);
}

// === Parameters in function call ===

TEST_F(ParserScannerTest, ParametersInFunctionCall1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo()\n"
        "}\n"
        "func foo(i int) {\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, 7, true)\n"
        "}\n"
        "func foo(i int, i2 int, b bool) bool {\n"
        "    return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, 7, true)\n"
        "}\n"
        "func foo(i int, i2 int, b bool) bool {\n"
        "    return 0\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, 7, true)\n"
        "}\n"
        "func foo(i int, b bool) bool {\n"
        "    return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, true)\n"
        "}\n"
        "func foo(i int, i2 int, b bool) bool {\n"
        "    return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo()\n"
        "}\n"
        "func foo(i int, i2 int, b bool) bool {\n"
        "    return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall7) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, 7, true)\n"
        "}\n"
        "func foo(i int, i2 int, b bool) (bool, int) {\n"
        "    return true\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall8) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    foo(4, 7, true)\n"
        "}\n"
        "func foo(i int, i2 int, b bool) (bool, int) {\n"
        "    return true, 0\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall9) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := foo()\n"
        "}\n"
        "func foo(str string) string {\n"
        "    return \"\"\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall10) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4 / foo()\n"
        "}\n"
        "func foo() int {\n"
        "   return 5.8\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, ParametersInFunctionCall11) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    bar(4, foo())\n"
        "}\n"
        "func bar(i int, b bool) {\n"
        "}\n"
        "func foo() int {\n"
        "    return 1\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE, false);
}

// === Test function main definition ===

TEST_F(ParserScannerTest, FunctionMain1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, FunctionMain2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main(i int) {\n"
        "    a := 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, FunctionMain3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main(i int, str string) {\n"
        "    a := 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, FunctionMain4) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() int {\n"
        "    a := 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, FunctionMain5) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() (string, int) {\n"
        "    a := 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_WRONG_PARAMETER_OR_RETURN_VALUE);
}

TEST_F(ParserScannerTest, FunctionMain6) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func foo(i int) {\n"
        "    a := 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}

// === Zero division ===

TEST_F(ParserScannerTest, ZeroDivision1) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4 / 0\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_DIVISION_BY_ZERO);
}

TEST_F(ParserScannerTest, ZeroDivision2) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4 / (5 - 5)\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_DIVISION_BY_ZERO);
}

TEST_F(ParserScannerTest, ZeroDivision3) {
    std::string inputStr = \
        "package main\n"
        "\n"
        "func main() {\n"
        "    a := 4 / foo()\n"
        "}\n"
        "func foo() int {\n"
        "    return 0\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_DIVISION_BY_ZERO);
}

TEST_F(ParserScannerTest, MultivalDefinition1) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "    b := 0\n"
        "    a, b := 5, 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_SUCCESS);
}

TEST_F(ParserScannerTest, MultivalDefinition2) {
    std::string inputStr = \
        "package main\n"
        "func main() {\n"
        "    a, b := 0, 5\n"
        "    a, b := 5, 6\n"
        "}\n";

    ComplexTest(inputStr, COMPILER_RESULT_ERROR_UNDEFINED_OR_REDEFINED_FUNCTION_OR_VARIABLE);
}
