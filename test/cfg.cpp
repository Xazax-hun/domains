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


TEST(Cfg, BasicCfg)
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

TEST(Cfg, CfgWithMoreNesting)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
iter {
  iter {
    translation(10, 0)
  };
  {
    translation(10, 0)
  } or {
    {
      translation(10, 0)
    } or {
      iter {
        rotation(0, 0, 90)
      }
    }
  }
})";
    std::string_view expected =
R"(digraph CFG {
  Node_0[label="init(50, 50, 50, 50)\ntranslation(10, 0)\n"]
  Node_1[label=""]
  Node_2[label="translation(10, 0)\n"]
  Node_3[label=""]
  Node_4[label="translation(10, 0)\n"]
  Node_5[label=""]
  Node_6[label="translation(10, 0)\n"]
  Node_7[label=""]
  Node_8[label="rotation(0, 0, 90)\n"]
  Node_9[label=""]
  Node_10[label=""]
  Node_11[label=""]
  Node_12[label=""]

  Node_0 -> Node_1
  Node_1 -> Node_2
  Node_2 -> Node_2
  Node_2 -> Node_3
  Node_3 -> Node_4
  Node_3 -> Node_5
  Node_4 -> Node_11
  Node_5 -> Node_6
  Node_5 -> Node_7
  Node_6 -> Node_10
  Node_7 -> Node_8
  Node_8 -> Node_8
  Node_8 -> Node_9
  Node_9 -> Node_10
  Node_10 -> Node_11
  Node_11 -> Node_1
  Node_11 -> Node_12
}
)";
    auto result = parseToCFG(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result.has_value());
    auto prettyPrintedCfg = print(result->root);
    EXPECT_EQ(prettyPrintedCfg, expected);
}

TEST(Cfg, RpoOrder)
{
    //     0
    //    / \  no multiline comment 
    //   1   2
    //   |   |
    //   |   3
    //    \ / 
    //     4
    CFG cfg;
    cfg.blocks.resize(5);
    cfg.addEdge(0, 1)
       .addEdge(0, 2)
       .addEdge(1, 4)
       .addEdge(2, 3)
       .addEdge(3, 4);

    RPOCompare compare(cfg);
    EXPECT_EQ(compare.getRpoPosition(0), 0);
    EXPECT_EQ(compare.getRpoPosition(1), 1);
    EXPECT_EQ(compare.getRpoPosition(2), 2);
    EXPECT_EQ(compare.getRpoPosition(3), 3);
    EXPECT_EQ(compare.getRpoPosition(4), 4);
}

TEST(Cfg, RpoOrder_Mirrored)
{
    //     0
    //    / \  no multiline comment 
    //   2   1
    //   |   |
    //   3   |
    //    \ / 
    //     4
    CFG cfg;
    cfg.blocks.resize(5);
    cfg.addEdge(0, 2)
       .addEdge(0, 1)
       .addEdge(1, 4)
       .addEdge(2, 3)
       .addEdge(3, 4);

    RPOCompare compare(cfg);
    EXPECT_EQ(compare.getRpoPosition(0), 0);
    EXPECT_EQ(compare.getRpoPosition(2), 1);
    EXPECT_EQ(compare.getRpoPosition(3), 2);
    EXPECT_EQ(compare.getRpoPosition(1), 3);
    EXPECT_EQ(compare.getRpoPosition(4), 4);
}

TEST(Cfg, RpoOrder_WithBackEdges)
{
    //      0  <----
    //     / \   | |
    //    1   2--| |
    //    |   |    |
    //    |   3----|
    //     \ / 
    //      4
    CFG cfg;
    cfg.blocks.resize(5);
    cfg.addEdge(0, 1)
       .addEdge(0, 2)
       .addEdge(1, 4)
       .addEdge(2, 3)
       .addEdge(2, 0)
       .addEdge(3, 4)
       .addEdge(3, 0);

    RPOCompare compare(cfg);
    EXPECT_EQ(compare.getRpoPosition(0), 0);
    EXPECT_EQ(compare.getRpoPosition(1), 1);
    EXPECT_EQ(compare.getRpoPosition(2), 2);
    EXPECT_EQ(compare.getRpoPosition(3), 3);
    EXPECT_EQ(compare.getRpoPosition(4), 4);
}

TEST(Cfg, RpoOrder_WithBackEdges_2)
{
    //      0  <----
    //     / \   | |
    // -->1   2--| |
    // |  |   |    |
    // |  |   3----|
    // |   \ / 
    // |----4
    CFG cfg;
    cfg.blocks.resize(5);
    cfg.addEdge(0, 1)
       .addEdge(0, 2)
       .addEdge(1, 4)
       .addEdge(2, 3)
       .addEdge(2, 0)
       .addEdge(3, 4)
       .addEdge(3, 0)
       .addEdge(4, 1);

    // TODO: is this actually the order we want?
    //       would we want to visit 1 earlier?
    RPOCompare compare(cfg);
    EXPECT_EQ(compare.getRpoPosition(0), 0);
    EXPECT_EQ(compare.getRpoPosition(2), 1);
    EXPECT_EQ(compare.getRpoPosition(3), 2);
    EXPECT_EQ(compare.getRpoPosition(4), 3);
    EXPECT_EQ(compare.getRpoPosition(1), 4);
}

// TODO: add property based tests,
//  * No unreachable nodes
//  * All next indices are valid
//  ...

} // anonymous