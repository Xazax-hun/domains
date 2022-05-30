#include "include/analyze.h"
#include "include/dataflow/solver.h"
#include "include/dataflow/domains/domain.h"
#include "include/dataflow/analyses/sign_analysis.h"
#include "include/dataflow/analyses/interval_analysis.h"

#include <unordered_map>

namespace 
{

template<Domain D, AnalysisFunc<D> getAnalysis>
AnalysisResult getResults(const CFG& cfg)
{
    auto results = getAnalysis(cfg);
    auto annotations = annotationsFromAnalysisResults(results, cfg);
    std::vector<Polygon> covered;
    for (auto state : results)
    {
        for (const auto& poly : state.covers())
            covered.push_back(poly);
    }
    
    return { std::move(annotations), std::move(covered)};
}

using AnalysisResultsFunc = AnalysisResult(*)(const CFG& cfg);

std::unordered_map<std::string_view, AnalysisResultsFunc> analyses = {
    {"sign", &getResults<Vec2Sign, getSignAnalysis> },
    {"primitive-interval", &getResults<Vec2Interval, getPrimitiveIntervalAnalysis> },
    {"interval", &getResults<Vec2Interval, getIntervalAnalysis> }
};

} // anonymous

std::optional<AnalysisResult> getAnalysisResults(std::string_view analysisName, const CFG& cfg)
{
    if (auto it = analyses.find(analysisName); it != analyses.end())
    {
        return it->second(cfg);
    }

    return {};
}

std::set<std::string_view> getListOfAnalyses()
{
    std::set<std::string_view> results;
    for (const auto& [name, _] : analyses)
    {
        results.insert(name);
    }
    return results;
}
