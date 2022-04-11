#include "include/eval.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/cfg.h"
#include "include/render.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <random>
#include <cmath>

#include <fmt/format.h>

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
        return Step{ Vec2 { in->nextPos.x + *t->x.value, in->nextPos.y + *t->y.value}, {}, {}, false };
    }

    Step operator()(Rotation* r) noexcept
    {
        Vec2 origin{*r->x.value, *r->y.value};
        Vec2 transformed { in->nextPos.x - origin.x, in->nextPos.y - origin.y };
        double rotRad = toRad(*r->deg.value);
        Vec2 rotated;
        rotated.x = transformed.x * cos(rotRad) - transformed.y * sin(rotRad);
        rotated.y = transformed.y * cos(rotRad) + transformed.x * sin(rotRad);
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
        if (cfg.blocks[current].nexts.empty())
            break;
        std::uniform_int_distribution<int> genNext(0, cfg.blocks[current].nexts.size() - 1);
        current = cfg.blocks[current].nexts[genNext(gen)];
    } while (true);
    return w;
}

bool runFile(std::string_view filePath, bool dumpCfg, bool svg)
{
    DiagnosticEmitter emitter(std::cout, std::cerr);
    std::ifstream file(filePath.data());
    std::stringstream fileContent;
    fileContent << file.rdbuf();
    Lexer lexer(std::move(fileContent).str(), emitter);
    auto tokens = lexer.lexAll();
    if (tokens.empty())
        return false;
    Parser parser(tokens, emitter);
    auto root = parser.parse();
    if (!root)
        return false;
    CFG cfg = createCfg(*root);
    if (dumpCfg)
        std::cout << print(cfg);
    Walk w = createRandomWalk(cfg);
    if (w.empty())
        return false;

    if (svg)
        std::cout << renderRandomWalkSVG(w) << "\n";
    else
    {
        for (auto step : w)
            std::cout << step.nextPos.x << " " << step.nextPos.y << "\n";
    }
    return true;
}
