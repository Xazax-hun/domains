#ifndef SIGN_DOMAIN_H
#define SIGN_DOMAIN_H

#include "include/dataflow/domain.h"

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
    Top,
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
};

bool operator==(SignDomain lhs, SignDomain rhs)
{
    return lhs.v == rhs.v;
}

bool operator<=(SignDomain lhs, SignDomain rhs)
{
    if (lhs.v == SignValue::Bottom)
        return true;

    if (rhs.v == SignValue::Top)
        return true;

    return false;
}

static_assert(Domain<SignDomain>);

#endif // SIGN_DOMAIN_H