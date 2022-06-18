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

std::optional<AnalysisResult> getAnalysisResults(std::string_view analysisName, const CFG& cfg);
std::set<std::string_view> getListOfAnalyses();

#endif // ANALYZE_H