#include <gtest/gtest.h>

#include "include/parser.h"

namespace
{

std::optional<ASTContext> parseString(std::string_view str, std::ostream& output)
{
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::string(str), emitter);
    auto tokens = lexer.lexAll();
    if (tokens.empty())
        return {};
    Parser parser(tokens, emitter);
    return parser.parse();
}

TEST(Parser, AllNodesParsed)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
iter {
  {
    translation(10, 0);
    iter {
      translation(10, 0)
    }
  } or {
    rotation(0, 0, 90)
  }
})";
    auto result = parseString(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result.has_value());
    auto prettyPrinted = print(result->getRoot());
    EXPECT_EQ(prettyPrinted, source);
}

TEST(Parser, EmptyAlternativeInOr)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
{
  translation(10, 0)
} or {

})";
    auto result = parseString(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result.has_value());
    auto prettyPrinted = print(result->getRoot());
    EXPECT_EQ(prettyPrinted, source);
}

TEST(Parser, EmptyInput)
{
    std::stringstream output;
    std::string_view source = "";
    auto result = parseString(source, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(output.str(), "[line 1] Error at end of file: 'init' expected at the beginning of the program.\n");
}

TEST(Parser, IllegalInit)
{
    {
        std::stringstream output;
        std::string_view source = "init(50, 50, -1, 0)";
        auto result = parseString(source, output);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(output.str(), "[line 1] Error at 'init': the width of the initial area cannot be negative.\n");
    }
    {
        std::stringstream output;
        std::string_view source = "init(50, 50, 0, -1)";
        auto result = parseString(source, output);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(output.str(), "[line 1] Error at 'init': the height of the initial area cannot be negative.\n");
    }
}

TEST(Parser, EmptyOr)
{
    std::stringstream output;
    std::string_view source = "init(50, 50, 50, 50); {} or {}";
    auto result = parseString(source, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(output.str(), "[line 1] Error at 'or': at most one alternative can be empty.\n");
}

TEST(Parser, TypoInOr)
{
    std::stringstream output;
    std::string_view source = "init(50, 50, 50, 50); {} 10 { translation(0, 0) }";
    auto result = parseString(source, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(output.str(), "[line 1] Error at '10': 'or' expected.\n");
}

TEST(Parser, EmptyLoop)
{
    std::stringstream output;
    std::string_view source = "init(50, 50, 50, 50); iter {}";
    auto result = parseString(source, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(output.str(), "[line 1] Error at 'iter': the body of 'iter' must not be empty.\n");
}

TEST(Parser, RedundantSemicolon)
{
    std::stringstream output;
    std::string_view source = "init(50, 50, 50, 50); iter { translation(0, 0); }";
    auto result = parseString(source, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(output.str(), "[line 1] Error at '}': redundant semicolon?\n");
}

TEST(Parser, RedundantSemicolon2)
{
    std::stringstream output;
    std::string_view source = "init(50, 50, 50, 50);";
    auto result = parseString(source, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(output.str(), "[line 1] Error at end of file: redundant semicolon?\n");
}

TEST(Parser, FromFuzzing)
{
    std::stringstream output;
    std::string_view source = "init";
    auto result = parseString(source, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(output.str(), "[line 1] Error at end of file: '(' expected.\n");
}

} // anonymous