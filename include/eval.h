#ifndef EVAL_H
#define EVAL_H

#include <optional>
#include <string_view>
#include <vector>

#include "include/utils.h"

struct Step {
    Vec2 pos;
    std::optional<Vec2> origin;
    std::optional<int> deg;
    bool init = false;
};

using Walk = std::vector<Step>;

class CFG;
Walk createRandomWalk(const CFG& cfg);

#endif // EVAL_H