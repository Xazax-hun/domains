
#include <gtest/gtest.h>
#include "analysis_test_support.h"
#include "include/dataflow/analyses/interval_analysis.h"

namespace
{
auto primitiveIntervalAnalyze = analyzeForTest<CFG, Vec2Interval,
                                              getPrimitiveIntervalAnalysis,
                                              intervalAnalysisToOperationAnnotations,
                                              intervalAnalysisToCoveredArea>;

auto intervalAnalyze = analyzeForTest<CFG, Vec2Interval,
                                      getIntervalAnalysis,
                                      intervalAnalysisToOperationAnnotations,
                                      intervalAnalysisToCoveredArea>;

TEST(IntervalAnalysis, PrimitiveAnalysis)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0))";
    std::string_view expected =
R"(init(50, 50, 50, 50) /* { x: [50, 100], y: [50, 100] } */;
translation(10, 0) /* { x: [60, 110], y: [50, 100] } */)";
    auto result = primitiveIntervalAnalyze(source, output);
    std::string annotatedSource = print(result->context.getRoot(), result->anns);
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
    auto result = primitiveIntervalAnalyze(source, output);
    std::string annotatedSource = print(result->context.getRoot(), result->anns);
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
    auto result = primitiveIntervalAnalyze(source, output);
    std::string annotatedSource = print(result->context.getRoot(), result->anns);
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
    auto result = primitiveIntervalAnalyze(source, output);
    std::string annotatedSource = print(result->context.getRoot(), result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(IntervalAnalysis, PrimitiveAnalysisDoesNotConverge)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
iter {
  translation(10, 0)
})";
    auto result = primitiveIntervalAnalyze(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->analysis.empty());
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
    auto result = intervalAnalyze(source, output);
    std::string annotatedSource = print(result->context.getRoot(), result->anns);
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
rotation(0, 0, 90) /* { x: [-100, -50], y: [70, inf] } */)";
    auto result = intervalAnalyze(source, output);
    std::string annotatedSource = print(result->context.getRoot(), result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(IntervalAnalysis, WidenLoopWithBranchAndRotate)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
iter {
  {
    translation(10, 0)
  } or {
    translation(0, 10)
  }
};
rotation(0, 0, 180))";
    std::string_view expected =
R"(init(50, 50, 50, 50) /* { x: [50, 100], y: [50, 100] } */;
iter {
  {
    translation(10, 0) /* { x: [60, inf], y: [50, inf] } */
  } or {
    translation(0, 10) /* { x: [50, inf], y: [60, inf] } */
  }
};
rotation(0, 0, 180) /* { x: [-inf, -50], y: [-inf, -50] } */)";
    auto result = intervalAnalyze(source, output);
    std::string annotatedSource = print(result->context.getRoot(), result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

} // anonymous