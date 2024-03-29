#ifndef SIGN_DOMAIN_H
#define SIGN_DOMAIN_H

#include "include/dataflow/domains/domain.h"

#include <cassert>

// TODO:
// Add more general building blocks for finite domains and
// post SignDomain to those facilities.

//     Top
//   /  |  \  no new line
//   N  Z  P
//   \  |  /
//    Bottom
enum class SignValue
{
    Top = 0,
    Bottom,
    Negative,
    Zero,
    Positive
};

inline constexpr SignValue toAbstract(int value) noexcept
{
    using enum SignValue;
    if (value < 0)
        return Negative;
    if (value > 0)
        return Positive;
    return Zero;
}

struct SignDomain
{
    using enum SignValue;
    SignValue v;

    explicit constexpr SignDomain(SignValue v) : v(v) {}
    explicit constexpr SignDomain(int v) : v(toAbstract(v)) {}

    static SignDomain bottom() { return SignDomain{ Bottom }; }
    SignDomain join(SignDomain other) const
    {
        if (v == other.v || other.v == Bottom)
            return *this;
        if (v == Bottom)
            return other;
        return SignDomain{ Top };
    }

    std::string_view toString() const
    {
        switch(v)
        {
            case Top:
                return "Top";
            case Bottom:
                return "Bottom";
            case Negative:
                return "Negative";
            case Zero:
                return "Zero";
            case Positive:
                return "Positive";
            default:
                assert(false);
                return "";
        }
    }

    std::vector<Polygon> covers() const
    {
        switch(v)
        {
            case Top:
                return {std::vector{Vec2{NEG_INF, 0}, Vec2{INF, 0}}};
            case Bottom:
                return {};
            case Negative:
                return {std::vector{Vec2{NEG_INF, 0}, Vec2{0, 0}}};
            case Zero:
                return {std::vector{Vec2{0, 0}}};
            case Positive:
                return {std::vector{Vec2{0, 0}, Vec2{INF, 0}}};
            default:
                assert(false);
                return {};
        }
    }
};

inline bool operator==(SignDomain lhs, SignDomain rhs) noexcept
{
    return lhs.v == rhs.v;
}

inline bool operator<=(SignDomain lhs, SignDomain rhs) noexcept
{
    if (lhs.v == SignValue::Bottom)
        return true;

    if (rhs.v == SignValue::Top)
        return true;

    return false;
}

static_assert(Domain<SignDomain>);

inline SignDomain operator-(SignDomain d) noexcept
{
    if (d.v == SignValue::Negative)
        return SignDomain{ SignValue::Positive };
    if (d.v == SignValue::Positive)
        return SignDomain{ SignValue::Negative };

    return d;
}

inline SignDomain operator+(SignDomain lhs, SignDomain rhs) noexcept
{
    using enum SignValue;
    static constexpr SignValue AdditionTable[][5] = 
    {
        // LHS/RHS,      Top,  Bottom, Negative, Zero,     Positive
        /* Top      */ { Top,  Top,    Top,      Top,      Top    },
        /* Bottom   */ { Top,  Bottom, Bottom,   Bottom,   Bottom },
        /* Negative */ { Top,  Bottom, Negative, Negative, Top    },
        /* Zero     */ { Top,  Bottom, Negative, Zero,     Positive},
        /* Positive */ { Top,  Bottom, Top,      Positive, Positive}
    };

    return SignDomain{ AdditionTable[static_cast<int>(lhs.v)][static_cast<int>(rhs.v)] };
}

#endif // SIGN_DOMAIN_H