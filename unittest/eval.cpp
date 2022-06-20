#include <gtest/gtest.h>

#include "include/parser.h"
#include "include/cfg.h"
#include "include/eval.h"

namespace
{

struct ParseResult
{
    Walk w;
    Parser parser; // Owns the nodes.
};

std::optional<ParseResult> getWalk(std::string_view str, std::ostream& output)
{
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::string(str), emitter);
    auto tokens = lexer.lexAll();
    if (tokens.empty())
        return {};
    Parser parser(tokens, emitter);
    auto root = parser.parse();
    if (!root)
        return {};
    auto cfg = CFG::createCfg(*root);
    auto w = createRandomWalk(cfg);
    return ParseResult{std::move(w), std::move(parser)};
}

constexpr double threshold = 1e-10;

double distSquared(Vec2 lhs, Vec2 rhs)
{
    double xdist = (lhs.x - rhs.x);
    double ydist = (lhs.y - rhs.y);
    return (xdist * xdist + ydist * ydist);
}

bool veryClose(double lhs, double rhs)
{
    return abs(lhs - rhs) < threshold;
}

bool veryClose(Vec2 lhs, Vec2 rhs)
{
    return distSquared(lhs, rhs) < threshold;
}

TEST(Eval, NoControlFlowWalk)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 0, 0, 0);
translation(10, 0);
rotation(0, 0, 90))";

    auto result = getWalk(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_EQ(result->w.size(), 3);
    Vec2 expected[] = { Vec2{50, 0}, Vec2{60, 0}, Vec2{0, 60}};
    EXPECT_TRUE(veryClose(result->w[0].pos, expected[0]));
    EXPECT_TRUE(veryClose(result->w[1].pos, expected[1]));
    EXPECT_TRUE(veryClose(result->w[2].pos, expected[2]));
    EXPECT_TRUE(result->w[0].init);
    EXPECT_FALSE(result->w[1].origin);
    EXPECT_FALSE(result->w[1].deg);
    EXPECT_TRUE(result->w[2].origin);
    EXPECT_TRUE(result->w[2].deg);
}

TEST(Eval, IterTest)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 0, 0, 0);
iter { translation(10, 0) })";

    auto result = getWalk(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_TRUE(!result->w.empty());
    EXPECT_TRUE(result->w[0].init);
    for (unsigned i = 1; i < result->w.size(); ++i)
    {
        EXPECT_FALSE(result->w[i].origin);
        EXPECT_FALSE(result->w[i].deg);
        EXPECT_TRUE(veryClose(result->w[i - 1].pos.x + 10, result->w[i].pos.x));
    }
}

TEST(Eval, OrTest)
{
    std::stringstream output;
    std::string_view source =
R"(init(50, 0, 0, 0);
{ translation(0, 10) } or { translation(10, 0) })";

    auto result = getWalk(source, output);
    EXPECT_TRUE(output.str().empty());
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->w.size() == 2);
    EXPECT_TRUE(result->w[0].init);
    EXPECT_FALSE(result->w[1].origin);
    EXPECT_FALSE(result->w[1].deg);
    EXPECT_TRUE(veryClose(distSquared(result->w[0].pos, result->w[1].pos), 10*10));
}

} // namespace