#include <gtest/gtest.h>

#include "include/parser.h"

namespace
{
struct ParseResult
{
    std::optional<Node> root;
    Parser parser; // Owns the nodes.
};

std::optional<ParseResult> parseString(std::string_view str, std::ostream& output)
{
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::string(str), emitter);
    auto tokens = lexer.lexAll();
    if (tokens.empty())
        return {};
    Parser parser(tokens, emitter);
    return ParseResult{parser.parse(), std::move(parser)};
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
    EXPECT_TRUE(result->root.has_value());
    auto prettyPrinted = print(*result->root);
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
    EXPECT_TRUE(result->root.has_value());
    auto prettyPrinted = print(*result->root);
    EXPECT_EQ(prettyPrinted, source);
}

TEST(Parser, EmptyInput)
{
    std::stringstream output;
    std::string_view source = "";
    auto result = parseString(source, output);
    EXPECT_EQ(output.str(), "[line 1] Error at end of file: 'init' expected at the beginning of the program\n");
}

TEST(Parser, EmptyOr)
{
    std::stringstream output;
    std::string_view source = "init(50, 50, 50, 50); {} or {}";
    auto result = parseString(source, output);
    EXPECT_EQ(output.str(), "[line 1] Error at 'or': at most one alternative can be empty\n");
}

TEST(Parser, EmptyLoop)
{
    std::stringstream output;
    std::string_view source = "init(50, 50, 50, 50); iter {}";
    auto result = parseString(source, output);
    EXPECT_EQ(output.str(), "[line 1] Error at 'iter': the body of 'iter' must not be empty\n");
}

} // anonymous