#include <gtest/gtest.h>
#include "analysis_test_support.h"
#include "include/dataflow/analyses/reachable_operations_analysis.h"

namespace
{
auto pastOpsAnalyze = analyzeForTest<CFG, StringSetDomain,
                                     getPastOperationsAnalysis,
                                     pastOperationsAnalysisToOperationAnnotations,
                                     pastOperationsAnalysisToCoveredArea>;

auto futureOpsAnalyze = analyzeForTest<ReverseCFG, StringSetDomain,
                                       getFutureOperationsAnalysis,
                                       futureOperationsAnalysisToOperationAnnotations,
                                       futureOperationsAnalysisToCoveredArea>;

TEST(ReachableOpsAnalysis, PastOperations)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
iter {
  translation(10, 0);
  rotation(0, 0, 90)
})";
    std::string_view expected =
R"(init(50, 50, 50, 50) /* {Init} */;
translation(10, 0) /* {Init, Translation} */;
iter {
  translation(10, 0) /* {Init, Rotation, Translation} */;
  rotation(0, 0, 90) /* {Init, Rotation, Translation} */
})";
    auto result = pastOpsAnalyze(source, output);
    std::string annotatedSource = print(result->root, result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

TEST(ReachableOpsAnalysis, FutureOperations)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 50, 50, 50);
translation(10, 0);
{
  translation(10, 0)
} or {
  rotation(0, 0, 90)
})";
    std::string_view expected =
R"(/* {Init, Rotation, Translation} */ init(50, 50, 50, 50);
/* {Rotation, Translation} */ translation(10, 0);
{
  /* {Translation} */ translation(10, 0)
} or {
  /* {Rotation} */ rotation(0, 0, 90)
})";
    auto result = futureOpsAnalyze(source, output);
    std::string annotatedSource = print(result->root, result->anns);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, annotatedSource);
}

} // anonymous namepsace
