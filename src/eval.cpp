#include "include/eval.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/cfg.h"
#include "include/render.h"

#include <random>
#include <cmath>

double toRad(double deg) { return deg / 180 * M_PI; }

struct StepEval {
    const Step* in;
    std::mt19937& gen;
    Step operator()(Init* i) noexcept
    {
        std::uniform_int_distribution<int> genX(*i->topX.value, *i->topX.value + *i->width.value);
        std::uniform_int_distribution<int> genY(*i->topY.value, *i->topY.value + *i->height.value);
        return Step{ Vec2 (genX(gen), genY(gen)), {}, {}, true};
    }

    Step operator()(Translation* t) noexcept
    {
        return Step{ Vec2 { in->pos.x + *t->x.value, in->pos.y + *t->y.value}, {}, {}, false };
    }

    Step operator()(Rotation* r) noexcept
    {
        Vec2 origin{*r->x.value, *r->y.value};
        Vec2 transformed { in->pos.x - origin.x, in->pos.y - origin.y };
        double rotRad = toRad(*r->deg.value);
        Vec2 rotated;
        rotated.x = round(transformed.x * cos(rotRad) - transformed.y * sin(rotRad));
        rotated.y = round(transformed.y * cos(rotRad) + transformed.x * sin(rotRad));
        rotated.x += origin.x;
        rotated.y += origin.y;
        return Step {
            rotated,
            origin,
            *r->deg.value,
            false
        };
    }
};

Walk createRandomWalk(const CFG& cfg)
{
    Walk w;
    if (!std::holds_alternative<Init*>(cfg.blocks.at(0).operations.at(0)))
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
            Step next;
            if (w.empty())
               next = std::visit(StepEval{nullptr, gen}, o);
            else
               next = std::visit(StepEval{&w.back(), gen}, o);
            w.push_back(next);
        }
        if (cfg.blocks[current].succs.empty())
            break;
        std::uniform_int_distribution<int> genNext(0, cfg.blocks[current].succs.size() - 1);
        current = cfg.blocks[current].succs[genNext(gen)];
    } while (true);
    return w;
}
