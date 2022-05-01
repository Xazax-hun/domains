#ifndef SIGN_DOMAIN_H
#define SIGN_DOMAIN_H

#include "include/dataflow/domain.h"

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

struct SignDomain
{
    SignValue v;

    static SignDomain bottom() { return { SignValue::Bottom }; }
    SignDomain merge(SignDomain other) const
    {
        if (v == other.v || other.v == SignValue::Bottom)
            return *this;
        if (v == SignValue::Bottom)
            return other;
        return { SignValue::Top };
    }

    std::string_view toString() const
    {
        switch(v)
        {
            case SignValue::Top:
                return "Top";
            case SignValue::Bottom:
                return "Bottom";
            case SignValue::Negative:
                return "Negative";
            case SignValue::Zero:
                return "Zero";
            case SignValue::Positive:
                return "Positive";
            default:
                assert(false);
                return "";
        }
    }
};

inline bool operator==(SignDomain lhs, SignDomain rhs)
{
    return lhs.v == rhs.v;
}

inline bool operator<=(SignDomain lhs, SignDomain rhs)
{
    if (lhs.v == SignValue::Bottom)
        return true;

    if (rhs.v == SignValue::Top)
        return true;

    return false;
}

static_assert(Domain<SignDomain>);

inline SignDomain negate(SignDomain d)
{
    if (d.v == SignValue::Negative)
        return SignDomain{ SignValue::Positive };
    if (d.v == SignValue::Positive)
        return SignDomain{ SignValue::Negative };

    return d;
}

inline SignDomain add(SignDomain lhs, SignDomain rhs)
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

    return SignDomain { AdditionTable[static_cast<int>(lhs.v)][static_cast<int>(rhs.v)] };
}

#endif // SIGN_DOMAIN_H