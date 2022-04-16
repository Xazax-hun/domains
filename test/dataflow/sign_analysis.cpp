#include <gtest/gtest.h>

#include "include/dataflow/analyses/sign_analysis.h"
#include "include/parser.h"
#include "include/cfg.h"

namespace
{
struct AnalysisResult
{
    CFG cfg;
    std::vector<Vec2Sign> analysis;
    Parser parser; // Owns the nodes.
};

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
    auto result = getSignAnalysis(cfg);
    return AnalysisResult{std::move(cfg), std::move(result), std::move(parser)};
}

using enum SignValue;

TEST(SignAnalysis, LinearProgram)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
rotation(0, 0, 0)
)";
    auto posPos = Vec2Sign{SignDomain{Positive}, SignDomain{Positive}};
    auto result = analyze(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(result->analysis[0], posPos);
}


TEST(SignAnalysis, Alternative)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
{ translation(10, 0) } or { translation(-10, 0) })";
    auto posPos = Vec2Sign{SignDomain{Positive}, SignDomain{Positive}};
    auto topPos = Vec2Sign{SignDomain{Top}, SignDomain{Positive}};
    auto result = analyze(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    // Start.
    EXPECT_EQ(result->analysis[0], posPos);
    // Left branch.
    EXPECT_EQ(result->analysis[1], posPos);
    // Right branch.
    EXPECT_EQ(result->analysis[2], topPos);
    // Merge.
    EXPECT_EQ(result->analysis[3], topPos);
}

TEST(SignAnalysis, Loop)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
iter { translation(0, 0) })";
    auto posPos = Vec2Sign{SignDomain{Positive}, SignDomain{Positive}};
    auto result = analyze(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    // Start.
    EXPECT_EQ(result->analysis[0], posPos);
    // Loop body.
    EXPECT_EQ(result->analysis[1], posPos);
    // After loop.
    EXPECT_EQ(result->analysis[2], posPos);
}

} // namespace
