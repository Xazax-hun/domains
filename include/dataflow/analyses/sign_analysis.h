#ifndef SIGN_ANALYSIS_H
#define SIGN_ANALYSIS_H

#include <vector>

#include "include/dataflow/domains/sign_domain.h"
#include "include/dataflow/domains/vec2_domain.h"
#include "include/cfg.h"

using Vec2Sign = Vec2Domain<SignDomain>;

std::vector<Vec2Sign> getSignAnalysis(const CFG& cfg);

Annotations signAnalysisToOperationAnnotations(const CFG& cfg,
                                               const std::vector<Vec2Sign>& results);

#endif // SIN_ANALYSIS_H
