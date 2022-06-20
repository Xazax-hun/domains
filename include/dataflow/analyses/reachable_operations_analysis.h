#ifndef REACHABLE_OPERATIONS_ANALYSIS_H
#define REACHABLE_OPERATIONS_ANALYSIS_H

#include <vector>
#include <string>

#include "include/dataflow/domains/powerset_domain.h"
#include "include/cfg.h"

using StringSetDomain = PowersetDomain<std::string>;

std::vector<StringSetDomain> getPastOperationsAnalysis(const CFG& cfg);
std::vector<StringSetDomain> getFutureOperationsAnalysis(const ReverseCFG& cfg);

Annotations pastOperationsAnalysisToOperationAnnotations(const CFG& cfg,
                                                         const std::vector<StringSetDomain>& results);

Annotations futureOperationsAnalysisToOperationAnnotations(const ReverseCFG& cfg,
                                                           const std::vector<StringSetDomain>& results);

inline std::vector<Polygon> pastOperationsAnalysisToCoveredArea(const CFG&,
    const std::vector<StringSetDomain>&)
{
    return {};
}

inline std::vector<Polygon> futureOperationsAnalysisToCoveredArea(const ReverseCFG&,
    const std::vector<StringSetDomain>&)
{
    return {};
}

#endif // REACHABLE_OPERATIONS_ANALYSIS_H 