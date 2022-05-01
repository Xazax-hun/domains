#include <gtest/gtest.h>

#include "include/dataflow/sign_domain.h"
#include "include/dataflow/vec2_domain.h"

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
}

} // anonymous