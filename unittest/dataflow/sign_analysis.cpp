#include <gtest/gtest.h>
#include "analysis_test_support.h"
#include "include/dataflow/analyses/sign_analysis.h"

namespace
{
auto signAnalyze = analyzeForTest<CFG, Vec2Sign,
                                  getSignAnalysis,
                                  signAnalysisToOperationAnnotations,
                                  signAnalysisToCoveredArea>;

using enum SignValue;

TEST(SignAnalysis, LinearProgram)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
rotation(0, 0, 0))";
    std::string_view expected =
R"(init(50, 50, 50, 50) /* { x: Positive, y: Positive } */;
translation(10, 0) /* { x: Positive, y: Positive } */;
rotation(0, 0, 0) /* { x: Positive, y: Positive } */)";
    auto result = signAnalyze(source, output);
    auto anns = signAnalysisToOperationAnnotations(result->cfg, result->analysis);
    std::string annotatedSource = print(result->context.getRoot(), anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(SignAnalysis, TraceAnalysis)
{
    std::string logged;
    struct Logger {
      void log(Operation op, Vec2Sign from, Vec2Sign to) {
        logBuffer += print(toNode(op));
        logBuffer += ": ";
        logBuffer += from.toString();
        logBuffer += " -> ";
        logBuffer += to.toString();
        logBuffer += '\n';
      }
      std::string& logBuffer;
    };
    Logger logger{logged};
    TransferTracer<Vec2Sign, SignTransfer, Logger> tracer{{}, logger};

    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
rotation(0, 0, 0))";
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::string(source), emitter);
    auto tokens = lexer.lexAll();
    EXPECT_FALSE(tokens.empty());
    Parser parser(tokens, emitter);
    auto ctxt = parser.parse();
    EXPECT_TRUE(ctxt);
    auto cfg = CFG::createCfg(ctxt->getRoot());
    auto results = solveMonotoneFramework<Vec2Sign, decltype(tracer)>(cfg, tracer);
    std::string_view expected =
R"(init(50, 50, 50, 50): { x: Bottom, y: Bottom } -> { x: Positive, y: Positive }
translation(10, 0): { x: Positive, y: Positive } -> { x: Positive, y: Positive }
rotation(0, 0, 0): { x: Positive, y: Positive } -> { x: Positive, y: Positive }
)";
    EXPECT_EQ(expected, logged);
}

TEST(SignAnalysis, Alternative)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
{
  translation(10, 0)
} or {
  translation(-10, 0)
})";
    std::string_view expected =
R"(init(50, 50, 50, 50) /* { x: Positive, y: Positive } */;
{
  translation(10, 0) /* { x: Positive, y: Positive } */
} or {
  translation(-10, 0) /* { x: Top, y: Positive } */
})";
    auto result = signAnalyze(source, output);
    auto anns = signAnalysisToOperationAnnotations(result->cfg, result->analysis);
    std::string annotatedSource = print(result->context.getRoot(), anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(SignAnalysis, Loop)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
iter {
  translation(0, 0)
})";
    auto posPos = Vec2Sign{SignDomain{Positive}, SignDomain{Positive}};
    auto result = signAnalyze(source, output);
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
