#ifndef PAIR_DOMAIN_H
#define PAIR_DOMAIN_H

#include "include/dataflow/domains/domain.h"

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

    Vec2Domain merge(const Vec2Domain& other) const
    {
        return Vec2Domain{x.merge(other.x), y.merge(other.y)};
    }

    Vec2Domain widen(const Vec2Domain transferredState) const requires WidenableDomain<D>
    {
        return {x.widen(transferredState.x), y.widen(transferredState.y)};
    }

    std::string toString() const
    {
        return fmt::format("{{ x: {}, y: {} }}", x.toString(), y.toString());
    }

    // TODO: This is failing. Is this a gcc bug?
    //       It does work when I move this static assert to unit tests.
    // static_assert(Domain<Vec2Domain>);
};


#endif // PAIR_DOMAIN_H