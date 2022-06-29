#ifndef INTERVAL_ANALYSIS_H
#define INTERVAL_ANALYSIS_H

#include <vector>

#include "include/dataflow/domains/interval_domain.h"
#include "include/dataflow/domains/vec2_domain.h"
#include "include/cfg.h"

using Vec2Interval = Vec2Domain<IntervalDomain>;

struct IntervalTransfer
{
    Vec2Interval operator()(Operation op, Vec2Interval preState) const;
};

// This is a deliberately inefficient/primitive implementation. It has hard time
// dealing with loops and kept for exposition only.
std::vector<Vec2Interval> getPrimitiveIntervalAnalysis(const CFG& cfg);

// Always widen during the fixed-point iteration.
std::vector<Vec2Interval> getIntervalAnalysis(const CFG& cfg);

Annotations intervalAnalysisToOperationAnnotations(const CFG& cfg,
                                                   const std::vector<Vec2Interval>& results);

std::vector<Polygon> intervalAnalysisToCoveredArea(const CFG& cfg,
                                                   const std::vector<Vec2Interval>& results);

// TODO: add variants of interval analysis:
// - Back-edge only widening
// - Loop unrolling
// - Narrowing
// - ...

#endif // INTERVAL_ANALYSIS_H