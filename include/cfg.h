#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "include/ast.h"
#include <queue>

using Operation = std::variant<const Init*, const Translation*, const Rotation*>;
inline Node toNode(Operation op) noexcept
{
    return std::visit([](const auto* node) noexcept -> Node { return node; }, op);
}

class BasicBlock
{
public:
    const std::vector<Operation>& operations() const noexcept { return ops; }
    const std::vector<int>& successors() const noexcept { return succs; }
    const std::vector<int>& predecessors() const noexcept { return preds; }

private:
    std::vector<Operation> ops;
    std::vector<int> succs;
    std::vector<int> preds;
    friend class CFG;
};

class CFG
{
public:
    const std::vector<BasicBlock>& blocks() const { return basicBlocks; }
    static CFG createCfg(Node root) noexcept;

private:
    CFG() = default;
    std::vector<BasicBlock> basicBlocks;
    CFG& addEdge(int from, int to);
    int newBlock();
    int addAstNode(int currentBlock, Node currentNode);

    friend class CFGTest;
};

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