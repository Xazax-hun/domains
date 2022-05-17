#ifndef INTERVAL_DOMAIN_H
#define INTERVAL_DOMAIN_H

#include <algorithm>
#include <cassert>

#include <fmt/format.h>

#include "include/dataflow/domains/domain.h"

struct IntervalDomain;
bool operator==(IntervalDomain lhs, IntervalDomain rhs);

struct IntervalDomain
{
    int min, max;

    explicit IntervalDomain(int num) : min(num), max(num) {}
    IntervalDomain(int min, int max) : min(min), max(max) {}

    static IntervalDomain bottom() { return {INF, NEG_INF}; }
    static IntervalDomain top() { return {NEG_INF, INF}; }

    IntervalDomain merge(IntervalDomain other) const
    {
        return {std::min(min, other.min), std::max(max, other.max)};
    }

    IntervalDomain widen(IntervalDomain transferredState) const
    {
        if (*this == bottom())
            return transferredState;
        int resultMin = [this, transferredState] {
            if (transferredState.min < min)
                return NEG_INF;
            return min;
        }();
        int resultMax = [this, transferredState]{
            if (transferredState.max > max)
                return INF;
            return max;
        }();
        return {resultMin, resultMax};
    }

    std::string toString() const
    {
        std::string minStr{ [this]() -> std::string {
                if (min == NEG_INF)
                    return "-inf";
                return std::to_string(min);
            }()
        };
        std::string maxStr{ [this]() -> std::string {
                if (max == INF)
                    return "inf";
                return std::to_string(max);
            }()
        };
        return fmt::format("[{}, {}]", minStr, maxStr);
    }
};

inline bool operator==(IntervalDomain lhs, IntervalDomain rhs)
{
    return lhs.min == rhs.min && lhs.max == rhs.max;
}

inline bool operator<=(IntervalDomain lhs, IntervalDomain rhs)
{
    return rhs.min <= lhs.min && rhs.max >= lhs.max;
}

static_assert(WidenableDomain<IntervalDomain>);

inline IntervalDomain operator-(IntervalDomain o)
{
    int minResult = [o] {
        if (o.max == INF)
            return NEG_INF;
        return -o.max;
    }();
    int maxResult = [o] {
        if (o.min == NEG_INF)
            return INF;
        return -o.min;
    }();
    return {minResult, maxResult};
}

inline IntervalDomain operator+(IntervalDomain lhs, IntervalDomain rhs)
{
    int resultMin = [lhs, rhs]{
        assert(lhs.min != INF && rhs.min != INF);
        if (lhs.min == NEG_INF || rhs.min == NEG_INF)
            return NEG_INF;
        return lhs.min + rhs.min;
    }();
    int resultMax = [lhs, rhs]{
        assert(lhs.max != NEG_INF && rhs.max != NEG_INF);
        if (lhs.max == INF || rhs.max == INF)
            return INF;
        return lhs.max + rhs.max;
    }();
    return {resultMin, resultMax};
}

#endif // INTERVAL_DOMAIN_H