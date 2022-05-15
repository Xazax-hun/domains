#include "include/render.h"

#include <sstream>
#include <cmath>

#ifdef HAVE_CAIRO
#include <cairo.h>
#include <cairo-svg.h> 


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

void renderRandomPath(cairo_t *cr, const Walk& w)
{
    // Draw the lines
    cairo_set_source_rgb(cr, 1, 0, 0);
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
    // Draw the dots
    for (auto step : w)
    {
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0, step.init ? 1 : 0, 0);
        cairo_arc(cr, step.pos.x, -step.pos.y, RADIUS, 0, 2 * M_PI);
        cairo_fill(cr);
    }

}

} // anonymous

std::string renderRandomWalkSVG(std::vector<Walk> walks)
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
        // Draw the axes. 
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_new_path(cr);
        cairo_move_to(cr, 0, -HEIGHT/2);
        cairo_line_to(cr, 0, HEIGHT);
        cairo_move_to(cr, -WIDTH/2, 0);
        cairo_line_to(cr, WIDTH, 0);
        cairo_stroke(cr);

        for (const auto& walk : walks)
        {
            renderRandomPath(cr, walk);
        }
        // Surface needs to be destroyed here.
    }

    return std::move(svgContent).str();
}

#else

std::string renderRandomWalkSVG(std::vector<Walk> walks)
{
    return "ERROR: compiled without cairo.";
}

#endif