#include "include/eval.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/cfg.h"
#include "include/render.h"

#include <random>
#include <cmath>

namespace
{
double toRad(double deg) noexcept { return deg / 180 * M_PI; }

struct StepEval {
    const Step* in;
    std::mt19937& gen;
    Step operator()(const Init* i) const noexcept
    {
        std::uniform_int_distribution<int> genX(*i->topX.value, *i->topX.value + *i->width.value);
        std::uniform_int_distribution<int> genY(*i->topY.value, *i->topY.value + *i->height.value);
        return Step{ Vec2 (genX(gen), genY(gen)), {}, {}, true};
    }

    Step operator()(const Translation* t) const noexcept
    {
        return Step{ in->pos + Vec2{ *t->x.value, *t->y.value }, {}, {}, false };
    }

    Step operator()(const Rotation* r) const noexcept
    {
        Vec2 origin{*r->x.value, *r->y.value};
        Vec2 toRotate { in->pos.x, in->pos.y };
        Vec2 rotated = rotate(toRotate, origin, *r->deg.value);
        return Step {
            rotated,
            origin,
            *r->deg.value,
            false
        };
    }
};
} // namespace anonymous

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

Walk createRandomWalk(const CFG& cfg)
{
    Walk w;
    if (!std::holds_alternative<const Init*>(cfg.blocks.at(0).operations.at(0)))
    {
        // TOOD: add error message.
        return w;
    }
    std::random_device rd;
    std::mt19937 gen(rd());
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
        std::uniform_int_distribution<int> randomSuccessor(0, cfg.blocks[current].succs.size() - 1);
        current = cfg.blocks[current].succs[randomSuccessor(gen)];
    } while (true);
    return w;
}
