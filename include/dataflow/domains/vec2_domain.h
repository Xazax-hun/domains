#ifndef PAIR_DOMAIN_H
#define PAIR_DOMAIN_H

#include "include/dataflow/domains/domain.h"

#include <algorithm>
#include <fmt/format.h>

// Product of two domain values, used to represent a point in 2d space.
template<Domain D>
struct Vec2Domain
{
    D x, y;

    bool operator==(const Vec2Domain& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator<=(const Vec2Domain& other) const
    {
        if (x == other.x)
            return y <= other.y;
        return x <= other.x;
    }

    static Vec2Domain bottom()
    {
        return Vec2Domain{D::bottom(), D::bottom()};
    }

    static Vec2Domain top() requires requires { { D::top() } -> std::same_as<D>; }
    {
        return Vec2Domain{D::top(), D::top()};
    }

    Vec2Domain join(const Vec2Domain& other) const
    {
        return Vec2Domain{x.join(other.x), y.join(other.y)};
    }

    Vec2Domain widen(const Vec2Domain transferredState) const requires WidenableDomain<D>
    {
        return {x.widen(transferredState.x), y.widen(transferredState.y)};
    }

    std::string toString() const
    {
        return fmt::format("{{ x: {}, y: {} }}", x.toString(), y.toString());
    }

    std::vector<Polygon> covers() const
    {
        auto xs = x.covers();
        auto ys = y.covers();

        assert(xs.size() == ys.size());
        std::vector<Polygon> polys;
        for(unsigned i = 0; i < xs.size(); ++i)
        {
            polys.emplace_back();
            bool first = true;
            for (Vec2 x : xs[i])
            {
                auto yCoords = ys[i];
                if (first)
                    first = false;
                else
                    std::reverse(yCoords.begin(), yCoords.end());
                for (Vec2 y : yCoords)
                    polys.back().emplace_back(x.x, y.x);
            }
        }

        return polys;
    }

    // TODO: This is failing. Is this a gcc bug?
    //       It does work when I move this static assert to unit tests.
    // static_assert(Domain<Vec2Domain>);
};


#endif // PAIR_DOMAIN_H