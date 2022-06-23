#include "include/dataflow/analyses/sign_analysis.h"

#include "include/dataflow/solver.h"

namespace
{
using enum SignValue;

struct TransferOperation
{
    // TODO: topX and topY are misleading, we should rename them.
    Vec2Sign operator()(const Init* init) const
    {
        SignDomain xSign{
            [topX = *init->topX.value, width = *init->width.value]() {
                if (topX > 0)
                    return Positive;
                if (topX + width < 0)
                    return Negative;
                if (topX == 0 && width == 0)
                    return Zero;

                return Top;
            }()
        };

        SignDomain ySign{
            [topY = *init->topY.value, height = *init->height.value]() {
                if (topY > 0)
                    return Positive;
                if (topY + height < 0)
                    return Negative;
                if (topY == 0 && height == 0)
                    return Zero;
                
                return Top;
            }()
        };

        return Vec2Sign{ xSign, ySign };
    }

    Vec2Sign operator()(const Translation* t) const
    {
        return Vec2Sign{preState.x + SignDomain{*t->x.value},
                        preState.y + SignDomain{*t->y.value}};
    }

    Vec2Sign operator()(const Rotation* r) const
    {
        int deg = *r->deg.value;
        if (deg % 360 == 0)
            return preState;

        if (*r->x.value == 0 && *r->y.value == 0)
        {
            if (deg % 360 == 270)
                return Vec2Sign{preState.y, -preState.x};
            if (deg % 180 == 0)
                return Vec2Sign{-preState.x, -preState.y};
            if (deg % 360 == 90)
                return Vec2Sign{-preState.y, preState.x};
        }
        return Vec2Sign{SignDomain{Top}, SignDomain{Top}};
    }
    
    Vec2Sign preState;
};

struct SignTransfer
{
    Vec2Sign operator()(Operation op, Vec2Sign preState) const
    {
        return std::visit(TransferOperation{preState}, op);
    }
};

} // anonymous


std::vector<Vec2Sign> getSignAnalysis(const CFG& cfg)
{
    return solveMonotoneFramework<Vec2Sign, SignTransfer>(cfg);
}

Annotations signAnalysisToOperationAnnotations(const CFG& cfg,
                                               const std::vector<Vec2Sign>& results)
{
    return allAnnotationsFromAnalysisResults<Vec2Sign, SignTransfer>(cfg, results);
}

std::vector<Polygon> signAnalysisToCoveredArea(const CFG& cfg,
                                               const std::vector<Vec2Sign>& results)
{
    return coveredAreaFromAnalysisResults<Vec2Sign, SignTransfer>(cfg, results);
}
