#ifndef INTERVAL_ANALYSIS_H
#define INTERVAL_ANALYSIS_H

#include <vector>

#include "include/dataflow/domains/interval_domain.h"
#include "include/dataflow/domains/vec2_domain.h"
#include "include/cfg.h"

using Vec2Interval = Vec2Domain<IntervalDomain>;

// This is a deliberately inefficient/primitive implementation. It has hard time
// dealing with loops and kept for exposition only.
std::vector<Vec2Interval> getPrimitiveIntervalAnalysis(const CFG& cfg);

// Always widen during the fixed-point iteration.
std::vector<Vec2Interval> getIntervalAnalysis(const CFG& cfg);


// TODO: add variants of interval analysis:
// - Widening
// - Loop unrolling
// - Narrowing
// - ...

#endif // INTERVAL_ANALYSIS_H