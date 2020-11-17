/** @file test_common.h
 *
 * IFJ20 compiler tests
 *
 * @brief Contains a common base for scanner/parser tests fixture.
 *
 * @author Ondřej Ondryáš (xondry02), FIT BUT
 */


#include "gtest/gtest.h"

extern "C" {
#include "scanner.h"
#include "tests_common.h"
}

#ifndef _STDINMOCKINGTEST_H
#define _STDINMOCKINGTEST_H

class StdinMockingScannerTest : public ::testing::Test {
protected:
    std::streambuf *cinBackup;
    std::stringbuf *buffer;

    void SetUp() override {
#if VERBOSE > 1
        std::cout << "[TEST SetUp]\n";
#endif
        compiler_result = COMPILER_RESULT_SUCCESS;
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
};

#endif
