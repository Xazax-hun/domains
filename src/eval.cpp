#include "include/eval.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/analysis.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <random>
#include <cmath>

#include <fmt/format.h>
#include <cairo.h>
#include <cairo-svg.h> 

struct Step {
    Vec2 nextPos;
    std::optional<Vec2> origin;
    std::optional<double> deg;
    bool init = false;
};

using Walk = std::vector<Step>;

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

cairo_status_t stringstream_writer(void *closure, const unsigned char *data, unsigned int length)
{
    static_cast<std::stringstream*>(closure)->write((const char*)data, length);
    return CAIRO_STATUS_SUCCESS;
}

std::string renderRandomWalkSVG(Walk w)
{
    constexpr int radius = 3;
    constexpr int width = 500;
    constexpr int height = 500;
    std::stringstream svgContent;
    {
        cairo_surface_t *surface = cairo_svg_surface_create_for_stream(stringstream_writer, &svgContent, width, height);
        Finally f1 { [=](){ cairo_surface_destroy(surface); } };
        cairo_t *cr = cairo_create(surface);
        Finally f2 { [=](){ cairo_destroy(cr); } };

        // Set background to white
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_new_path(cr);
        cairo_rectangle (cr, 0, 0, width, height);
        cairo_fill(cr);
        // Put origo on the middle.
        cairo_translate (cr, width/2, height/2);
        // Draw the axes. 
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_new_path(cr);
        cairo_move_to(cr, 0, -height/2);
        cairo_line_to(cr, 0, height);
        cairo_move_to(cr, -width/2, 0);
        cairo_line_to(cr, width, 0);
        cairo_stroke(cr);
        // Draw the lines
        cairo_set_source_rgb(cr, 1, 0, 0);
        for (unsigned i = 1; i < w.size(); ++i)
        {
            cairo_new_path(cr);
            if (w[i].origin)
            {
                double xdiff = w[i].origin->x - w[i].nextPos.x;
                double ydiff = w[i].origin->y - w[i].nextPos.y;
                double dist = sqrt(xdiff * xdiff + ydiff * ydiff);
                double degPrev = atan2(-w[i-1].nextPos.y, w[i-1].nextPos.x);
                double degCur = atan2(-w[i].nextPos.y, w[i].nextPos.x);
                cairo_arc(cr, w[i].origin->x, -w[i].origin->y, dist, degCur, degPrev);
                cairo_stroke(cr);
            }
            else
            {
                cairo_move_to(cr, w[i - 1].nextPos.x, -w[i - 1].nextPos.y);
                cairo_line_to(cr, w[i].nextPos.x, -w[i].nextPos.y);
                cairo_stroke(cr);
            }
        }
        // Draw the dots
        for (unsigned i = 0; i < w.size(); ++i)
        {
            cairo_new_path(cr);
            cairo_set_source_rgb(cr, 0, w[i].init ? 1 : 0, 0);
            cairo_arc(cr, w[i].nextPos.x, -w[i].nextPos.y, radius, 0, 2 * M_PI);
            cairo_fill(cr);
        }
        // Surface needs to be destroyed here.
    }

    return std::move(svgContent).str();
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
        print(cfg);
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
