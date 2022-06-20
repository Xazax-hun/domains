#include "include/analyze.h"
#include "include/dataflow/solver.h"
#include "include/dataflow/domains/domain.h"
#include "include/dataflow/analyses/sign_analysis.h"
#include "include/dataflow/analyses/interval_analysis.h"
#include "include/dataflow/analyses/reachable_operations_analysis.h"

#include <unordered_map>

namespace 
{

template<CfgConcept CFG, Domain D, AnalysisFunc<D, CFG> getAnalysis,
         AnnotatorFunc<D, CFG> AF, VisualizerFunc<D, CFG> VF>
AnalysisResult getResults(const CFG& cfg)
{
    auto results = getAnalysis(cfg);
    if (results.empty())
        return { false, {}, {} };

    auto annotations = AF(cfg, results);
    std::vector<Polygon> covered = VF(cfg, results);
    
    return { true, std::move(annotations), std::move(covered)};
}

template <CfgConcept CFG>
using AnalysisResultsFunc = AnalysisResult(*)(const CFG& cfg);

std::unordered_map<std::string_view, AnalysisResultsFunc<CFG>> forwardAnalyses = {
    {
        "sign", &getResults<CFG, Vec2Sign,
                            getSignAnalysis,
                            signAnalysisToOperationAnnotations,
                            signAnalysisToCoveredArea>
    },
    {
        "primitive-interval", &getResults<CFG, Vec2Interval,
                                          getPrimitiveIntervalAnalysis,
                                          intervalAnalysisToOperationAnnotations,
                                          intervalAnalysisToCoveredArea>
    },
    {
        "interval", &getResults<CFG, Vec2Interval,
                                getIntervalAnalysis,
                                intervalAnalysisToOperationAnnotations,
                                intervalAnalysisToCoveredArea>
    },
    {
        "past-operations", &getResults<CFG, StringSetDomain,
                                getPastOperationsAnalysis,
                                pastOperationsAnalysisToOperationAnnotations,
                                pastOperationsAnalysisToCoveredArea>
    }
};

std::unordered_map<std::string_view, AnalysisResultsFunc<ReverseCFG>> backwardAnalyses = {
    {
        "future-operations", &getResults<ReverseCFG, StringSetDomain,
                                getFutureOperationsAnalysis,
                                futureOperationsAnalysisToOperationAnnotations,
                                futureOperationsAnalysisToCoveredArea>
    }
};

} // anonymous

std::optional<AnalysisResult> getAnalysisResults(std::string_view analysisName, const CFG& cfg)
{
    if (auto it = forwardAnalyses.find(analysisName); it != forwardAnalyses.end())
        return it->second(cfg);

    if (auto it = backwardAnalyses.find(analysisName); it != backwardAnalyses.end())
    {
        ReverseCFG revCfg(cfg);
        return it->second(revCfg);
    }

    return {};
}

std::set<std::string_view> getListOfAnalyses()
{
    std::set<std::string_view> results;
    for (const auto& [name, _] : forwardAnalyses)
        results.insert(name);

    for (const auto& [name, _] : backwardAnalyses)
        results.insert(name);

    return results;
}
