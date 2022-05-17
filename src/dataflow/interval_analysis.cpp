#include "include/dataflow/analyses/interval_analysis.h"

#include <algorithm>

#include "include/dataflow/solver.h"
#include "include/eval.h"

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

    Vec2Interval operator()(const Rotation* r) const
    {
        // TODO: In some cases we could give tighter bounds
        //       for these.
        if (preState.x.max == INF ||
            preState.x.min == NEG_INF ||
            preState.y.max == INF ||
            preState.y.min == NEG_INF)
            return Vec2Interval::top();

        // The two intervals describe a rectangle aligned with the axes:
        //    
        //  +--------+
        //  |        |
        //  |        |
        //  +--------+
        //
        // To get the rotated region, we rotate all the corners and calculate
        // the bounding box of the result:
        //
        //  +---------+
        //  |   /¯--__|
        //  |  /     /|
        //  | /     / |
        //  |¯---__/  |
        //  +---------+
        //
        Vec2 corners[]{ Vec2{preState.x.min, preState.y.min},
                        Vec2{preState.x.min, preState.y.max},
                        Vec2{preState.x.max, preState.y.min},
                        Vec2{preState.x.max, preState.y.max} };
        Vec2 origin{*r->x.value, *r->y.value};
        for (auto& toRotate : corners)
        {
            toRotate = rotate(toRotate, origin, *r->deg.value);
        }
        auto newX = std::minmax_element(std::begin(corners), std::end(corners), [](Vec2 lhs, Vec2 rhs) {
            return lhs.x < rhs.x;
        });
        auto newY = std::minmax_element(std::begin(corners), std::end(corners), [](Vec2 lhs, Vec2 rhs) {
            return lhs.y < rhs.y;
        });
        return Vec2Interval{ IntervalDomain{ newX.first->x, newX.second->x },
                             IntervalDomain { newY.first->y, newY.second->y } };
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

std::vector<Vec2Interval> getIntervalAnalysis(const CFG& cfg)
{
    return solveMonotoneFrameworkWithWidening<Vec2Interval, IntervalTransfer>(cfg);
}
