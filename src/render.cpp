#include "include/render.h"
#include "include/dataflow/domains/domain.h"

#ifdef HAVE_CAIRO

#include <sstream>
#include <numbers>

#include <cairo.h>
#include <cairo-svg.h> 

#include <random>

namespace
{
// Render settings
constexpr int RADIUS = 3;
constexpr int WIDTH = 500;
constexpr int HEIGHT = 500;

cairo_status_t stringstream_writer(void *closure, const unsigned char *data, unsigned int length)
{
    static_cast<std::stringstream*>(closure)->write((const char*)data, length);
    return CAIRO_STATUS_SUCCESS;
}

// Pick easily distinguishable colors for the first 20,
// random colors afterwards.
struct RGB { double r, g, b; };
class ColorPicker
{
public:
    ColorPicker() : gen(rd()), genComp(0, 255) {}

    RGB next()
    {
        if (current < sizeof(colors)/sizeof(colors[0]))
            return colors[current++];

        return RGB { genComp(gen) / 255., genComp(gen) / 255., genComp(gen) / 255.};
    }
private:
    static constexpr RGB colors[] = {
        {230 / 255., 25 / 255., 75 / 255.},
        {60 / 255., 180 / 255., 75 / 255.},
        {0 / 255., 130 / 255., 200 / 255.},
        {245 / 255., 130 / 255., 48 / 255.},
        {145 / 255., 30 / 255., 180 / 255.},
        {70 / 255., 240 / 255., 240 / 255.},
        {240 / 255., 50 / 255., 230 / 255.},
        {210 / 255., 245 / 255., 60 / 255.},
        {250 / 255., 190 / 255., 212 / 255.},
        {0 / 255., 128 / 255., 128 / 255.},
        {220 / 255., 190 / 255., 255 / 255.},
        {170 / 255., 110 / 255., 40 / 255.},
        {255 / 255., 250 / 255., 200 / 255.},
        {255 / 255., 250 / 255., 200 / 255.},
        {128 / 255., 0 / 255., 0 / 255.},
        {170 / 255., 255 / 255., 195 / 255.},
        {128 / 255., 128 / 255., 0 / 255.},
        {255 / 255., 215 / 255., 180 / 255.},
        {0 / 255., 0 / 255., 128 / 255.},
        {255 / 255., 225 / 255., 25 / 255.},
        {128 / 255., 128 / 255., 128 / 255.}
    };

    unsigned current = 0;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<int> genComp;
};

void renderRandomPath(cairo_t *cr, const Walk& w, RGB color, bool dotsOnly)
{
    // Draw the lines
    if (!dotsOnly)
    {
        cairo_set_source_rgb(cr, color.r, color.g, color.b);
        for (unsigned i = 1; i < w.size(); ++i)
        {
            cairo_new_path(cr);
            if (w[i].origin)
            {
                int xdiff = w[i].origin->x - w[i].pos.x;
                int ydiff = w[i].origin->y - w[i].pos.y;
                double dist = sqrt(xdiff * xdiff + ydiff * ydiff);
                double degPrev = atan2(-w[i-1].pos.y, w[i-1].pos.x);
                double degCur = atan2(-w[i].pos.y, w[i].pos.x);
                cairo_arc(cr, w[i].origin->x, -w[i].origin->y, dist, degCur, degPrev);
                cairo_stroke(cr);
            }
            else
            {
                cairo_move_to(cr, w[i - 1].pos.x, -w[i - 1].pos.y);
                cairo_line_to(cr, w[i].pos.x, -w[i].pos.y);
                cairo_stroke(cr);
            }
        }
    }
    // Draw the dots
    for (auto step : w)
    {
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0, step.init ? 1 : 0, 0);
        cairo_arc(cr, step.pos.x, -step.pos.y, RADIUS, 0, 2 * std::numbers::pi_v<double>);
        cairo_fill(cr);
    }

}

int clipWidth(int pos) { return pos == INF ? WIDTH / 2 : (pos == NEG_INF ? -WIDTH / 2 : pos); }
int clipHeight(int pos) { return pos == INF ? HEIGHT / 2 : (pos == NEG_INF ? -HEIGHT / 2 : pos); }

void renderCoveredArea(cairo_t *cr, const std::vector<Polygon>& inferred)
{
    // Set background to grey
    cairo_set_source_rgb(cr, 0.75, 0.75, 0.75);
    for (const Polygon& p : inferred)
    {
        cairo_new_path(cr);
        bool first = true;
        for (Vec2 dot : p)
        {
            if (first)
            {
                cairo_move_to(cr, clipWidth(dot.x), -clipHeight(dot.y));
                first = false;
                continue;
            }
            cairo_line_to(cr, clipWidth(dot.x), -clipHeight(dot.y));
        }
        cairo_close_path(cr);
        cairo_fill(cr);
    }
}

} // anonymous

std::string renderRandomWalkSVG(const std::vector<Walk>& walks, const std::vector<Polygon>& inferred, bool dotsOnly)
{
    std::stringstream svgContent;
    {
        cairo_surface_t *surface = cairo_svg_surface_create_for_stream(stringstream_writer, &svgContent, WIDTH, HEIGHT);
        Finally f1 { [=](){ cairo_surface_destroy(surface); } };
        cairo_t *cr = cairo_create(surface);
        Finally f2 { [=](){ cairo_destroy(cr); } };

        // Set background to white
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_new_path(cr);
        cairo_rectangle (cr, 0, 0, WIDTH, HEIGHT);
        cairo_fill(cr);
        // Put origo on the middle.
        cairo_translate (cr, WIDTH/2, HEIGHT/2);

        // Render the result of the analysis.
        renderCoveredArea(cr, inferred);

        // Draw the axes. 
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_new_path(cr);
        cairo_move_to(cr, 0, -HEIGHT/2);
        cairo_line_to(cr, 0, HEIGHT);
        cairo_move_to(cr, -WIDTH/2, 0);
        cairo_line_to(cr, WIDTH, 0);
        cairo_stroke(cr);

        ColorPicker picker;
        for (const auto& walk : walks)
        {
            renderRandomPath(cr, walk, picker.next(), dotsOnly);
        }
        // Surface needs to be destroyed here.
    }

    return std::move(svgContent).str();
}

#else

std::string renderRandomWalkSVG(const std::vector<Walk>& /*walks*/,
                                const std::vector<Polygon>& /*inferred*/,
                                bool /*dotsOnly*/)
{
    return "ERROR: compiled without cairo.";
}

#endif
