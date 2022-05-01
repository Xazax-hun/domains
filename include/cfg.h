#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "include/ast.h"
#include <queue>

using Operation = std::variant<const Init*, const Translation*, const Rotation*>;
Node toNode(Operation op) noexcept;

struct BasicBlock
{
    std::vector<Operation> operations;
    std::vector<int> succs;
    std::vector<int> preds;
};

struct CFG
{
    std::vector<BasicBlock> blocks;
    CFG& addEdge(int from, int to);
};

CFG createCfg(Node node) noexcept;

class RPOCompare
{
public:
    explicit RPOCompare(const CFG& cfg);

    bool operator()(int lhsBlockId, int rhsBlockId) const
    {
        return rpoOrder[lhsBlockId] < rpoOrder[rhsBlockId];
    }

    int getRpoPosition(int node) const { return rpoOrder[node]; }
private:
    std::vector<int> rpoOrder;
};

class RPOWorklist
{
public:
    explicit RPOWorklist(const CFG&);
    void enqueue(int node) noexcept;
    void enqueueSuccessors(int node) noexcept;
    int dequeue() noexcept;
    bool empty() const noexcept { return worklist.empty(); }
private:
    using Worklist = std::priority_queue<int, std::vector<int>, RPOCompare>;
    const CFG& cfg;
    RPOCompare comparator; // Must be declared before the worklist.
    Worklist worklist;
    std::vector<bool> queued;
};

std::string print(const CFG& cfg) noexcept;

#endif // ANALYSIS_H