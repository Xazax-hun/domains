#include "include/analyze.h"
#include "include/dataflow/solver.h"
#include "include/dataflow/domains/domain.h"
#include "include/dataflow/analyses/sign_analysis.h"
#include "include/dataflow/analyses/interval_analysis.h"

#include <unordered_map>

namespace 
{

template<Domain D, AnalysisFunc<D> getAnalysis>
Annotations getResults(const CFG& cfg)
{
    auto results = getAnalysis(cfg);
    return annotationsFromAnalysisResults(results, cfg);
}

using AnalysisAnnotationsFunc = Annotations(*)(const CFG& cfg);

std::unordered_map<std::string_view, AnalysisAnnotationsFunc> analyses = {
    {"sign", &getResults<Vec2Sign, getSignAnalysis> },
    {"primitive-interval", &getResults<Vec2Interval, getPrimitiveIntervalAnalysis> },
    {"interval", &getResults<Vec2Interval, getIntervalAnalysis> }
};

} // anonymous

std::optional<Annotations> getAnalysisResults(std::string_view analysisName, const CFG& cfg)
{
    if (auto it = analyses.find(analysisName); it != analyses.end())
    {
        return it->second(cfg);
    }

    return {};
}

std::vector<std::string_view> getListOfAnalyses()
{
    std::vector<std::string_view> results;
    for (const auto& [name, _] : analyses)
    {
        results.push_back(name);
    }
    return results;
}
