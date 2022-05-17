#ifndef RENDER_H
#define RENDER_H

#include "include/eval.h"

#include <vector>

std::string renderRandomWalkSVG(const std::vector<Walk>& walks,
    const std::vector<Polygon>& inferred, bool dotsOnly = false);

#endif // RENDER_H