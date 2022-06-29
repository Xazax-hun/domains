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
        int degree = *r->deg.value;
        // Rotation by the multiple of 360 degrees will not change the state.
        if (degree % 360 == 0)
            return preState;

        // When the rotation is the multiple of 90 degrees, we can easily handle
        // the rotation and it will not interfere with the infinite bounds.
        // First translate the state so the rotation's center is at the origo.
        // Then do the rotation as if inf and -inf were just regular numbers.
        // Then undo the translation.
        Vec2 origin{*r->x.value, *r->y.value};
        Vec2Interval toRotate{preState.x + IntervalDomain{-origin.x},
                              preState.y + IntervalDomain{-origin.y}};
        if (degree % 360 == 270)
        {
            return Vec2Interval{toRotate.y + IntervalDomain{origin.x},
                                -toRotate.x + IntervalDomain{origin.y}};
        }
        if (degree % 180 == 0)
        {
            return Vec2Interval{-toRotate.x + IntervalDomain{origin.x},
                                -toRotate.y + IntervalDomain{origin.y}};
        }
        if (degree % 360 == 90)
        {
            return Vec2Interval{-toRotate.y + IntervalDomain{origin.x},
                                toRotate.x + IntervalDomain{origin.y}};
        }

        // TODO: when only some of the bounds are infinite, we might be able to
        //       preserve some information.
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
        for (auto& toRotate : corners)
            toRotate = rotate(toRotate, origin, degree);

        auto newX = std::minmax_element(std::begin(corners), std::end(corners), [](Vec2 lhs, Vec2 rhs) {
            return lhs.x < rhs.x;
        });
        auto newY = std::minmax_element(std::begin(corners), std::end(corners), [](Vec2 lhs, Vec2 rhs) {
            return lhs.y < rhs.y;
        });
        return Vec2Interval{ IntervalDomain{ newX.first->x, newX.second->x },
                             IntervalDomain{ newY.first->y, newY.second->y } };
    }
    
    Vec2Interval preState;
};

} // anonymous

Vec2Interval IntervalTransfer::operator()(Operation op, Vec2Interval preState) const
{
    return std::visit(TransferOperation{preState}, op);
}

std::vector<Vec2Interval> getPrimitiveIntervalAnalysis(const CFG& cfg)
{
    return solveMonotoneFramework<Vec2Interval, IntervalTransfer>(cfg);
}

std::vector<Vec2Interval> getIntervalAnalysis(const CFG& cfg)
{
    return solveMonotoneFrameworkWithWidening<Vec2Interval, IntervalTransfer>(cfg);
}

Annotations intervalAnalysisToOperationAnnotations(const CFG& cfg,
                                                   const std::vector<Vec2Interval>& results)
{
    return allAnnotationsFromAnalysisResults<Vec2Interval, IntervalTransfer>(cfg, results);
}

std::vector<Polygon> intervalAnalysisToCoveredArea(const CFG& cfg,
                                                   const std::vector<Vec2Interval>& results)
{
    return coveredAreaFromAnalysisResults<Vec2Interval, IntervalTransfer>(cfg, results);
}
