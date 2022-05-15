
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
    auto result = F(cfg);
    auto anns = annotationsFromAnalysisResults(result, cfg);
    return AnalysisResult{std::move(cfg), std::move(result), *root, std::move(anns), std::move(parser)};
}

TEST(IntervalAnalysis, PrimitiveAnalysis)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0))";
    std::string_view expected =
R"(init(50, 50, 50, 50);
translation(10, 0) /* { x: [60, 110], y: [50, 100] } */)";
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
R"(init(50, 50, 50, 50);
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

} // anonymous