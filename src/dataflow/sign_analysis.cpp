#include "include/dataflow/analyses/sign_analysis.h"

#include <cmath>

#include "include/dataflow/solver.h"

namespace
{
using enum SignValue;

SignDomain toAbstract(int value)
{
    if (value < 0)
        return SignDomain{ Negative };
    if (value > 0)
        return SignDomain{ Positive };
    if (std::isnan(value))
        return SignDomain{ Top };
    return SignDomain{ Zero };
}

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
        SignDomain xTranslate = toAbstract(*t->x.value);
        SignDomain yTranslate = toAbstract(*t->y.value);
        return Vec2Sign{add(preState.x, xTranslate),
                        add(preState.y, yTranslate)};
    }

    Vec2Sign operator()(const Rotation* r) const
    {
        if (*r->x.value == 0 && *r->y.value == 0)
        {
            int deg = *r->deg.value;
            if (deg % 360 == 0)
                return preState;
            if (deg % 360 == 270)
                return Vec2Sign{preState.y, negate(preState.x)};
            if (deg % 180 == 0)
                return Vec2Sign{negate(preState.x), negate(preState.y)};
            if (deg % 360 == 90)
                return Vec2Sign{negate(preState.y), preState.x};
        }
        return Vec2Sign{SignDomain{Top}, SignDomain{Top}};
    }
    
    Vec2Sign preState;
};

struct SignTransfer
{
    Vec2Sign operator()(Vec2Sign preState, Operation o) const
    {
        return std::visit(TransferOperation{preState}, o);
    }
};

} // anonymous


std::vector<Vec2Sign> getSignAnalysis(const CFG& cfg)
{
    return solveMonotoneFramework<Vec2Sign, SignTransfer>(cfg);
}
