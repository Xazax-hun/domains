#include <gtest/gtest.h>

#include <string>

#include "include/dataflow/domains/sign_domain.h"
#include "include/dataflow/domains/interval_domain.h"
#include "include/dataflow/domains/vec2_domain.h"
#include "include/dataflow/domains/powerset_domain.h"

namespace
{
using enum SignValue;

TEST(Domains, SignDomain)
{
    SignDomain bottom = SignDomain::bottom();
    SignDomain positive{Positive};
    SignDomain negative{Negative};
    SignDomain zero{Zero};
    SignDomain top{Top};

    EXPECT_EQ(positive, positive);
    EXPECT_TRUE(bottom <= negative);
    EXPECT_TRUE(zero <= top);
    EXPECT_EQ(zero.merge(zero), zero);
    EXPECT_EQ(negative.merge(positive), top);
    EXPECT_EQ(positive.merge(negative), top);
    EXPECT_EQ(top.merge(negative), top);
    EXPECT_EQ(negative.merge(top), top);
    EXPECT_EQ(bottom.merge(negative), negative);
    EXPECT_EQ(negative.merge(bottom), negative);


    EXPECT_EQ(negative.toString(), "Negative");
    EXPECT_EQ(positive.toString(), "Positive");
    EXPECT_EQ(zero.toString(), "Zero");
    EXPECT_EQ(top.toString(), "Top");
    EXPECT_EQ(bottom.toString(), "Bottom");
}

TEST(Domains, IntervalDomain)
{
    IntervalDomain bottom = IntervalDomain::bottom();
    IntervalDomain top = IntervalDomain::top();
    IntervalDomain singleton = IntervalDomain{5};
    IntervalDomain smallRangeA = IntervalDomain{0, 10};
    IntervalDomain smallRangeB = IntervalDomain{11, 20};
    IntervalDomain largeRange = IntervalDomain{-100, 100};

    // Ordering
    EXPECT_LE(bottom, top);
    EXPECT_LE(bottom, singleton);
    EXPECT_LE(bottom, smallRangeA);
    EXPECT_LE(singleton, smallRangeA);
    EXPECT_LE(smallRangeA, largeRange);
    EXPECT_LE(smallRangeB, largeRange);
    EXPECT_LE(largeRange, top);
    EXPECT_FALSE(smallRangeA <= smallRangeB);
    EXPECT_FALSE(smallRangeB <= smallRangeA);
    EXPECT_FALSE(singleton <= smallRangeB);
    EXPECT_FALSE(smallRangeB <= singleton);

    // Merging
    {
        EXPECT_EQ(bottom.merge(singleton), singleton);
        EXPECT_EQ(bottom.merge(smallRangeA), smallRangeA);
        EXPECT_EQ(smallRangeA.merge(bottom), smallRangeA);
        IntervalDomain mergedSmallsExpected{0, 20};
        EXPECT_EQ(smallRangeA.merge(smallRangeB), mergedSmallsExpected);
        EXPECT_EQ(largeRange.merge(top), top);
        EXPECT_EQ(top.merge(largeRange), top);
    }

    // Widening
    {
        EXPECT_EQ(largeRange.widen(smallRangeA), largeRange);
        EXPECT_EQ(smallRangeA.widen(largeRange), top);
        IntervalDomain bumpMax{smallRangeA.min, smallRangeA.max + 1};
        IntervalDomain wideningExpected{smallRangeA.min, INF}; 
        EXPECT_EQ(smallRangeA.widen(bumpMax), wideningExpected);
        EXPECT_EQ(bottom.widen(largeRange), largeRange);
    }

    // Arithmetic
    {
        EXPECT_EQ(singleton + singleton, IntervalDomain{10});
        EXPECT_EQ(-singleton, IntervalDomain{-5});
        EXPECT_EQ(singleton + top, top);
        EXPECT_EQ(-top, top);
        IntervalDomain negateExpected {-20, -11};
        EXPECT_EQ(-smallRangeB, negateExpected);
        IntervalDomain addExpected1 {5, 15};
        EXPECT_EQ(smallRangeA + singleton, addExpected1);
        EXPECT_EQ(singleton + smallRangeA, addExpected1);
        IntervalDomain addExpected2 {11, 30};
        EXPECT_EQ(smallRangeA + smallRangeB, addExpected2);
        EXPECT_EQ(smallRangeB + smallRangeA, addExpected2);
    }

    // Printing
    {
        EXPECT_EQ(singleton.toString(), "[5, 5]");
        EXPECT_EQ(smallRangeA.toString(), "[0, 10]");
        EXPECT_EQ(top.toString(), "[-inf, inf]");
        EXPECT_EQ(bottom.toString(), "[inf, -inf]");
    }
}

TEST(Domains, Vec2SignsDomain)
{
    using Vec2Signs = Vec2Domain<SignDomain>;
    static_assert(Domain<Vec2Signs>);

    Vec2Signs bottom = Vec2Signs::bottom();
    Vec2Signs posNeg{SignDomain{Positive}, SignDomain{Negative}};
    Vec2Signs posPos{SignDomain{Positive}, SignDomain{Positive}};
    Vec2Signs negPos{SignDomain{Negative}, SignDomain{Positive}};
    Vec2Signs posTop{SignDomain{Positive}, SignDomain{Top}};
    Vec2Signs topPos{SignDomain{Top}, SignDomain{Positive}};
    Vec2Signs topTop{SignDomain{Top}, SignDomain{Top}};

    EXPECT_EQ(bottom, bottom);
    EXPECT_TRUE(posPos <= posTop);
    EXPECT_FALSE(posPos <= posNeg);
    EXPECT_TRUE(posPos <= topTop);
    EXPECT_EQ(posNeg.merge(posPos), posTop);
    EXPECT_EQ(posPos.merge(posNeg), posTop);
    EXPECT_EQ(negPos.merge(posPos), topPos);
    EXPECT_EQ(posPos.merge(negPos), topPos);
    EXPECT_EQ(topTop.merge(posNeg), topTop);
    EXPECT_EQ(posNeg.merge(topTop), topTop);
    EXPECT_EQ(posNeg.merge(bottom), posNeg);
    EXPECT_EQ(bottom.merge(posNeg), posNeg);

    EXPECT_EQ(posNeg.toString(), "{ x: Positive, y: Negative }");
}

TEST(Domains, Vec2IntervalDomain)
{
    using Vec2Interval = Vec2Domain<IntervalDomain>;
    static_assert(WidenableDomain<Vec2Interval>);

    Vec2Interval singleton{IntervalDomain{5}, IntervalDomain{5}};
    Vec2Interval range{IntervalDomain{0, 10}, IntervalDomain{0, 10}};

    EXPECT_EQ(singleton.widen(range), Vec2Interval::top());
}

TEST(Domains, PowersetDomain)
{
    using StringSetDomain = PowersetDomain<std::string>;
    static_assert(Domain<StringSetDomain>);

    StringSetDomain bottom = StringSetDomain::bottom();
    StringSetDomain a{"a"};
    StringSetDomain b{"b"};
    StringSetDomain ab{"a", "b"};

    EXPECT_EQ(bottom, bottom);
    EXPECT_TRUE(bottom <= a);
    EXPECT_TRUE(bottom <= b);
    EXPECT_TRUE(a <= ab);
    EXPECT_TRUE(b <= ab);
    EXPECT_EQ(a.merge(b), ab);
    EXPECT_EQ(b.merge(a), ab);
    EXPECT_EQ(b.merge(b), b);
    EXPECT_EQ(bottom.merge(b), b);

    EXPECT_EQ(ab.toString(), "{a, b}");
    EXPECT_EQ(bottom.toString(), "{}");
}

} // anonymous