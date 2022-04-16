#include "include/dataflow/analyses/sign_analysis.h"

#include <cmath>

#include "include/dataflow/solver.h"

namespace
{

SignDomain toAbstract(int value)
{
    if (value < 0)
        return SignDomain{ SignValue::Negative };
    if (value > 0)
        return SignDomain{ SignValue::Positive };
    if (std::isnan(value))
        return SignDomain{ SignValue::Top };
    return SignDomain{ SignValue::Zero };
}

struct TransferOperation
{
    // TODO: topX and topY are misleading, we should rename them.
    Vec2Sign operator()(Init* init) const
    {
        SignDomain xSign{SignValue::Top};
        int topX = *init->topX.value;
        int width = *init->width.value;
        if (topX > 0)
            xSign = SignDomain{SignValue::Positive};
        else if (topX + width < 0)
            xSign = SignDomain{SignValue::Negative};
        else if (topX == 0 && width == 0)
            xSign = SignDomain{SignValue::Zero};

        SignDomain ySign{SignValue::Top};
        int topY = *init->topY.value;
        int height = *init->height.value;
        if (topY > 0)
            ySign = SignDomain{SignValue::Positive};
        else if (topY + height < 0)
            ySign = SignDomain{SignValue::Negative};
        else if (topY == 0 && height == 0)
            ySign = SignDomain{SignValue::Zero};

        return Vec2Sign{ xSign, ySign };
    }

    Vec2Sign operator()(Translation* t) const
    {
        SignDomain xTranslate = toAbstract(*t->x.value);
        SignDomain yTranslate = toAbstract(*t->y.value);
        return Vec2Sign{add(preState.x, xTranslate),
                        add(preState.y, yTranslate)};
    }

    Vec2Sign operator()(Rotation* r) const
    {
        if (*r->x.value == 0 && *r->y.value == 0)
        {
            int deg = *r->deg.value;
            if (deg % 360 == 0)
                return preState;
            if (deg % 270 == 0)
                return Vec2Sign{preState.y, negate(preState.x)};
            if (deg % 180 == 0)
                return Vec2Sign{negate(preState.x), negate(preState.y)};
            if (deg % 90 == 0)
                return Vec2Sign{negate(preState.y), preState.x};
        }
        return Vec2Sign{SignDomain{SignValue::Top}, SignDomain{SignValue::Top}};
    }
    
    Vec2Sign preState;
};

struct SignTransfer
{
    Vec2Sign operator()(Vec2Sign preState, Operation o)
    {
        return std::visit(TransferOperation{preState}, o);
    }
};

} // anonymous


std::vector<Vec2Sign> getSignAnalysis(const CFG& cfg)
{
    return solveMonotoneFramework<Vec2Sign, SignTransfer>(cfg);
}
