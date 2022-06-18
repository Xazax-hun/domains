
#include <gtest/gtest.h>

#include "include/dataflow/analyses/interval_analysis.h"
#include "include/dataflow/solver.h"
#include "include/analyze.h"
#include "include/parser.h"
#include "include/cfg.h"

namespace
{
struct AnalysisResult
{
    CFG cfg;
    std::vector<Vec2Interval> analysis;
    Node root;
    Annotations anns;
    Parser parser; // Owns the nodes.
};

// TODO: deduplicate among other analysis tests.
template <AnalysisFunc<Vec2Interval> F>
std::optional<AnalysisResult> analyze(std::string_view str, std::ostream& output)
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
    auto results = F(cfg);
    auto anns = intervalAnalysisToOperationAnnotations(cfg, results);
    return AnalysisResult{std::move(cfg), std::move(results), *root, std::move(anns), std::move(parser)};
}

TEST(IntervalAnalysis, PrimitiveAnalysis)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0))";
    std::string_view expected =
R"(init(50, 50, 50, 50) /* { x: [50, 100], y: [50, 100] } */;
translation(10, 0) /* { x: [60, 110], y: [50, 100] } */)";
    auto result = analyze<getPrimitiveIntervalAnalysis>(source, output);
    std::string annotatedSource = print(result->root, result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(IntervalAnalysis, PrimitiveAnalysisRotation)
{
    std::stringstream output;
    std::string_view source =
R"(init(20, 20, 50, 50);
rotation(0, 0, 90))";
    std::string_view expected =
R"(init(20, 20, 50, 50) /* { x: [20, 70], y: [20, 70] } */;
rotation(0, 0, 90) /* { x: [-70, -20], y: [20, 70] } */)";
    auto result = analyze<getPrimitiveIntervalAnalysis>(source, output);
    std::string annotatedSource = print(result->root, result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(IntervalAnalysis, PrimitiveAnalysisRotation2)
{
    std::stringstream output;
    std::string_view source =
R"(init(20, 20, 50, 50);
rotation(0, 0, 180))";
    std::string_view expected =
R"(init(20, 20, 50, 50) /* { x: [20, 70], y: [20, 70] } */;
rotation(0, 0, 180) /* { x: [-70, -20], y: [-70, -20] } */)";
    auto result = analyze<getPrimitiveIntervalAnalysis>(source, output);
    std::string annotatedSource = print(result->root, result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(IntervalAnalysis, PrimitiveAnalysisRotation3)
{
    std::stringstream output;
    std::string_view source =
R"(init(20, 20, 50, 50);
rotation(0, 0, 360))";
    std::string_view expected =
R"(init(20, 20, 50, 50) /* { x: [20, 70], y: [20, 70] } */;
rotation(0, 0, 360) /* { x: [20, 70], y: [20, 70] } */)";
    auto result = analyze<getPrimitiveIntervalAnalysis>(source, output);
    std::string annotatedSource = print(result->root, result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(IntervalAnalysis, WidenSimpleLoop)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
iter{
  translation(10, 0)
}
)";
    std::string_view expected =
R"(init(50, 50, 50, 50) /* { x: [50, 100], y: [50, 100] } */;
translation(10, 0) /* { x: [60, 110], y: [50, 100] } */;
iter {
  translation(10, 0) /* { x: [70, inf], y: [50, 100] } */
})";
    auto result = analyze<getIntervalAnalysis>(source, output);
    std::string annotatedSource = print(result->root, result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(IntervalAnalysis, WidenSimpleLoopAndRotate)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
iter{
  translation(10, 0)
};
rotation(0, 0, 90))";
    std::string_view expected =
R"(init(50, 50, 50, 50) /* { x: [50, 100], y: [50, 100] } */;
translation(10, 0) /* { x: [60, 110], y: [50, 100] } */;
iter {
  translation(10, 0) /* { x: [70, inf], y: [50, 100] } */
};
rotation(0, 0, 90) /* { x: [-inf, inf], y: [-inf, inf] } */)";
    auto result = analyze<getIntervalAnalysis>(source, output);
    std::string annotatedSource = print(result->root, result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

} // anonymous