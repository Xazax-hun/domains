#include "include/render.h"

#include <sstream>
#include <cmath>

#include <cairo.h>
#include <cairo-svg.h> 


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