#include <gtest/gtest.h>

#include <algorithm>

#include "include/lexer.h"
#include "include/utils.h"

namespace
{
using enum TokenType;

std::vector<Token> lexString(std::string s, std::ostream& output)
{
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::move(s), emitter);
    return lexer.lexAll();
}

TEST(Lexer, TestEmptyInput)
{
    {
        std::stringstream output;
        auto tokenList = lexString("", output);
        TokenType tokenTypes[] = {END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](const Token& t, TokenType type) { return t.type == type; }));
        EXPECT_TRUE(output.str().empty());
    }
    {
        std::stringstream output;
        auto tokenList = lexString("  \n\t\n", output);
        TokenType tokenTypes[] = {END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](const Token& t, TokenType type) { return t.type == type; }));
        EXPECT_TRUE(output.str().empty());
    }
}

TEST(Lexer, TestAllTokens)
{
    std::stringstream output;
    auto tokenList = lexString(R"({}(),;50 init translation rotation iter or)", output);
    TokenType tokenTypes[] = {LEFT_BRACE, RIGHT_BRACE, LEFT_PAREN, RIGHT_PAREN, COMMA, SEMICOLON, NUMBER,
                              INIT, TRANSLATION, ROTATION, ITER, OR, END_OF_FILE};
    EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                [](const Token& t, TokenType type) { return t.type == type; }));
    EXPECT_TRUE(output.str().empty());
}

TEST(Lexer, TestNumbers)
{
    std::stringstream output;
    auto tokenList = lexString("0 50 -0 -50", output);
    TokenType tokenTypes[] = {NUMBER, NUMBER, NUMBER, NUMBER, END_OF_FILE};
    EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                [](const Token& t, TokenType type) { return t.type == type; }));
    EXPECT_TRUE(output.str().empty());
    EXPECT_EQ(*tokenList[0].value, 0);
    EXPECT_EQ(*tokenList[1].value, 50);
    EXPECT_EQ(*tokenList[2].value, 0);
    EXPECT_EQ(*tokenList[3].value, -50);
}

TEST(Lexer, TestComments)
{
    std::stringstream output;
    auto tokenList = lexString("0 // the rest is ignored\n\n//so is this\n  // and this", output);
    TokenType tokenTypes[] = {NUMBER, END_OF_FILE};
    EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                [](const Token& t, TokenType type) { return t.type == type; }));
    EXPECT_EQ(*tokenList[0].value, 0);
    EXPECT_TRUE(output.str().empty());
}

TEST(Lexer, TestMultiLineComments)
{
    std::stringstream output;
    auto tokenList = lexString("/* foo */ 0 /* the rest * is */  /* ignored\n\n so is this\n  // and this */", output);
    TokenType tokenTypes[] = {NUMBER, END_OF_FILE};
    EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                [](const Token& t, TokenType type) { return t.type == type; }));
    EXPECT_EQ(*tokenList[0].value, 0);
    EXPECT_TRUE(output.str().empty());
}

TEST(Lexer, ErrorMessages)
{
    std::stringstream output;
    auto maybeTokens = lexString("|", output);
    EXPECT_TRUE(maybeTokens.empty());
    EXPECT_EQ("[line 1] Error : Unexpected token: '|'.\n", output.str());
}


} // namespace