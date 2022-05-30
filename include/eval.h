#ifndef EVAL_H
#define EVAL_H

#include <optional>
#include <string_view>
#include <vector>

#include "include/utils.h"

// TODO: Instead of optional fields would a variant work better?
struct Step {
    Vec2 pos;
    std::optional<Vec2> origin;
    std::optional<int> deg;
    bool init = false;
};

using Walk = std::vector<Step>;

class CFG;

// Loopiness specifies the weight of choosing back edges.
// Loopiness == 1 means that back edges are as likely to
// be picked as any other edge. Loopiness == n means that
// the back edges are n times as likely to be picked.
Walk createRandomWalk(const CFG& cfg, int loopiness = 1);

Vec2 rotate(Vec2 toRotate, Vec2 origin, int degree);

#endif // EVAL_H