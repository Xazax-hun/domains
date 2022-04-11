#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "include/ast.h"

using Operation = std::variant<Init*, Translation*, Rotation*>;

struct BasicBlock
{
    std::vector<Operation> operations;
    std::vector<int> nexts;
};

struct CFG
{
    std::vector<BasicBlock> blocks;
};

CFG createCfg(Node node) noexcept;

std::string print(const CFG& cfg);

#endif // ANALYSIS_H