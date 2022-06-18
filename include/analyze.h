#ifndef ANALYZE_H
#define ANALYZE_H

#include "include/cfg.h"

#include <string_view>
#include <set>

struct AnalysisResult
{
    bool converged = false;
    Annotations annotations;
    std::vector<Polygon> covered;
};

// Returns an empty optional when no analysis exists under the name specified.
std::optional<AnalysisResult> getAnalysisResults(std::string_view analysisName, const CFG& cfg);
std::set<std::string_view> getListOfAnalyses();

#endif // ANALYZE_H