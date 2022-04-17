#ifndef ANALYZE_H
#define ANALYZE_H

#include "include/cfg.h"

std::optional<Annotations> getAnalysisResults(std::string analysisName, const CFG& cfg);
std::vector<std::string> getListOfAnalyses();

#endif // ANALYZE_H