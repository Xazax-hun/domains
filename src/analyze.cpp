#include "include/analyze.h"
#include "include/dataflow/domain.h"
#include "include/dataflow/solver.h"
#include "include/dataflow/analyses/sign_analysis.h"

#include <unordered_map>

namespace 
{

template<Domain D>
using AnalysisFunc = std::vector<D>(*)(const CFG&);

template<Domain D, AnalysisFunc<D> getAnalysis>
Annotations getResults(const CFG& cfg)
{
    auto results = getAnalysis(cfg);
    return annotationsFromAnalysisResults(results, cfg);
}

using AnalysisAnnotationsFunc = Annotations(*)(const CFG& cfg);

std::unordered_map<std::string, AnalysisAnnotationsFunc> analyses = {
    {"sign", &getResults<Vec2Sign, getSignAnalysis> }
};

} // anonymous

std::optional<Annotations> getAnalysisResults(std::string analysisName, const CFG& cfg)
{
    if (auto it = analyses.find(analysisName); it != analyses.end())
    {
        return it->second(cfg);
    }

    return {};
}

std::vector<std::string> getListOfAnalyses()
{
    std::vector<std::string> results;
    for (const auto& [name, _] : analyses)
    {
        results.push_back(name);
    }
    return results;
}
