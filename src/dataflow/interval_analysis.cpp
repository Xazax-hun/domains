#include "include/dataflow/analyses/interval_analysis.h"

#include "include/dataflow/solver.h"

namespace
{
struct TransferOperation
{
    Vec2Interval operator()(const Init* init) const
    {
        int x = *init->topX.value;
        int y = *init->topY.value;
        int w = *init->width.value;
        int h = *init->height.value;
        return Vec2Interval{ IntervalDomain{x, x + w}, IntervalDomain{y, y + h} };
    }

    Vec2Interval operator()(const Translation* t) const
    {
        return Vec2Interval{preState.x + IntervalDomain{*t->x.value},
                            preState.y + IntervalDomain{*t->y.value}};
    }

    Vec2Interval operator()(const Rotation*) const
    {
        // TODO: do the actual calculation and give some bounds.
        return Vec2Interval::top();
    }
    
    Vec2Interval preState;
};

struct IntervalTransfer
{
    Vec2Interval operator()(Vec2Interval preState, Operation o) const
    {
        return std::visit(TransferOperation{preState}, o);
    }
};
} // anonymous

std::vector<Vec2Interval> getPrimitiveIntervalAnalysis(const CFG& cfg)
{
    return solveMonotoneFramework<Vec2Interval, IntervalTransfer>(cfg);
}