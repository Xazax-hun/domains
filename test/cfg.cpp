#include <gtest/gtest.h>

#include "include/parser.h"
#include "include/cfg.h"

namespace
{

struct ParseResult
{
    CFG root;
    Parser parser; // Owns the nodes.
};

std::optional<ParseResult> parseToCFG(std::string_view str, std::ostream& output)
{
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::string(str), emitter);
    auto tokens = lexer.lexAll();
    if (tokens.empty())
        return {};
    Parser parser(tokens, emitter);
    auto root = parser.parse();
    if (!root)
        return {};
    auto cfg = createCfg(*root);
    return ParseResult{std::move(cfg), std::move(parser)};
}


TEST(Parser, BasicCfg)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
iter {
  {
    translation(10, 0)
  } or {
    rotation(0, 0, 90)
  }
})";
    std::string_view expected =
R"(digraph CFG {
  Node_0[label="init(50, 50, 50, 50)\ntranslation(10, 0)\n"]
  Node_1[label=""]
  Node_2[label="translation(10, 0)\n"]
  Node_3[label="rotation(0, 0, 90)\n"]
  Node_4[label=""]
  Node_5[label=""]

  Node_0 -> Node_1
  Node_1 -> Node_2
  Node_1 -> Node_3
  Node_2 -> Node_4
  Node_3 -> Node_4
  Node_4 -> Node_1
  Node_4 -> Node_5
}
)";
    auto result = parseToCFG(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result.has_value());
    auto prettyPrintedCfg = print(result->root);
    EXPECT_EQ(prettyPrintedCfg, expected);
}

// TODO: add property based tests,
//  * No unreachable nodes
//  * All next indices are valid
//  ...

} // anonymous