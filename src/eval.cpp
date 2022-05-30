#include "include/eval.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/cfg.h"

#include <algorithm>
#include <random>
#include <numbers>
#include <set>

#include <fmt/format.h>

namespace
{
double toRad(double deg) noexcept { return deg / 180 * std::numbers::pi_v<double>; }

struct StepEval {
    const Step* in;
    std::mt19937& gen;
    Step operator()(const Init* i) const noexcept
    {
        std::uniform_int_distribution<int> genX(*i->topX.value, *i->topX.value + *i->width.value);
        std::uniform_int_distribution<int> genY(*i->topY.value, *i->topY.value + *i->height.value);
        return Step{ Vec2{ genX(gen), genY(gen) }, {}, {}, true};
    }

    Step operator()(const Translation* t) const noexcept
    {
        return Step{ in->pos + Vec2{ *t->x.value, *t->y.value }, {}, {}, false };
    }

    Step operator()(const Rotation* r) const noexcept
    {
        Vec2 origin{*r->x.value, *r->y.value};
        Vec2 toRotate{ in->pos.x, in->pos.y };
        Vec2 rotated = rotate(toRotate, origin, *r->deg.value);
        return Step {
            rotated,
            origin,
            *r->deg.value,
            false
        };
    }
};
} // anonymous namespace

Vec2 rotate(Vec2 toRotate, Vec2 origin, int degree)
{
    toRotate -= origin;
    double rotRad = toRad(degree);
    Vec2 rotated{
        static_cast<int>(round(toRotate.x * cos(rotRad) - toRotate.y * sin(rotRad))),
        static_cast<int>(round(toRotate.y * cos(rotRad) + toRotate.x * sin(rotRad)))
    };
    rotated += origin;
    return rotated;
}

namespace
{
std::set<std::pair<int, int>> detectBackEdges(const CFG& cfg, int current, const std::set<int>& visited)
{
    // Only do back-edge detection when we first encounter a node.
    if (visited.contains(current))
        return {};

    std::set<std::pair<int, int>> ret;
    for(auto succ : cfg.blocks[current].succs)
    {
        // In every walk, in order to take the back edge of a loop,
        // we first need to enter the loop. Thus, if the target of the
        // edge is already visited, we see a back edge.
        if (visited.contains(succ))
            ret.insert(std::make_pair(current, succ));
    }
    return ret;
}

int getNextBlock(std::mt19937& gen, const CFG& cfg, int current,
                      const std::set<std::pair<int, int>>& backEdges, int loopiness)
{
    std::vector<int> successors = cfg.blocks[current].succs;
    auto it = std::partition(successors.begin(), successors.end(),
        [&backEdges, current](int succ) {
            return !backEdges.contains(std::make_pair(current, succ));
        });

    int regularEdgeNum = it - successors.begin();
    int backEdgeNum = successors.end() - it;
    std::uniform_int_distribution<int> shouldTakeBackEdge(1, regularEdgeNum + backEdgeNum * loopiness);
    if (shouldTakeBackEdge(gen) <= regularEdgeNum)
    {
        int next = std::uniform_int_distribution<int>(0, regularEdgeNum - 1)(gen);
        assert(next < static_cast<int>(successors.size()));
        return successors[next];
    }

    int next = std::uniform_int_distribution<int>(regularEdgeNum, regularEdgeNum + backEdgeNum - 1)(gen);
    assert(next < static_cast<int>(successors.size()));
    return successors[next];
}
} // anonymous namespace

Walk createRandomWalk(const CFG& cfg, int loopiness)
{
    Walk w;
    if (!std::holds_alternative<const Init*>(cfg.blocks.at(0).operations.at(0)))
    {
        // TODO: add error message.
        return w;
    }
    std::random_device rd;
    std::mt19937 gen(rd());

    // Discover what edges are back edges as we go to correctly 
    // account for loopiness when choosing the next block.
    std::set<int> visited;
    std::set<std::pair<int, int>> backEdges;

    int current = 0;
    do 
    {
        for (Operation o : cfg.blocks[current].operations)
        {
            if (w.empty())
               w.push_back(std::visit(StepEval{nullptr, gen}, o));
            else
               w.push_back(std::visit(StepEval{&w.back(), gen}, o));
        }
        if (cfg.blocks[current].succs.empty())
            break;
        backEdges.merge(detectBackEdges(cfg, current, visited));
        visited.insert(current);
        current = getNextBlock(gen, cfg, current, backEdges, loopiness);
    } while (true);

    return w;
}
