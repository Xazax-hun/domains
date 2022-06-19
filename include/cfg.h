#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "include/ast.h"
#include <queue>
#include <ranges>
#include <sstream>

// Elements of a basic block, cannot represent control flow.
using Operation = std::variant<const Init*, const Translation*, const Rotation*>;

inline Node toNode(Operation op) noexcept
{
    return std::visit([](const auto* node) noexcept -> Node { return node; }, op);
}

template<typename T>
concept CfgBlockConcept = requires(T a)
{
    { a.operations() } -> std::ranges::range;
    { a.successors() } -> std::ranges::range;
    { a.predecessors() } -> std::ranges::range;
};

template<typename T>
concept CfgConcept = requires(T a)
{
    { a.blocks() } -> std::ranges::range;
    { *a.blocks().begin() } -> CfgBlockConcept;
};

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

static_assert(CfgBlockConcept<BasicBlock>);

class CFG
{
public:
    // The first block is the start block. The last block is the end block.
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

static_assert(CfgConcept<CFG>);

class ReverseBasicBlock
{
public:
    ReverseBasicBlock(const BasicBlock& bb, int blockNum)
        : bb(bb), blockNum(blockNum) {}

    auto operations() const noexcept { return std::views::reverse(bb.operations()); }
    auto successors() const noexcept {
        return bb.predecessors() | std::views::transform([this](int idx) {
                   return blockNum - 1 - idx;
               });
    }
    auto predecessors() const noexcept {
        return bb.successors() | std::views::transform([this](int idx) {
                   return blockNum - 1 - idx;
               });
    }

private:
    const BasicBlock& bb;
    int blockNum;
};

static_assert(CfgBlockConcept<ReverseBasicBlock>);

class ReverseCFG
{
public:
    ReverseCFG(const CFG& cfg) : cfg(cfg) {}

    auto blocks() const {
        return std::views::reverse(cfg.blocks()) |
               std::views::transform([this](const BasicBlock& bb) {
                  return ReverseBasicBlock(bb, cfg.blocks().size());
               });
    }

private:
    const CFG& cfg;
};

static_assert(CfgConcept<ReverseCFG>);

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

template<CfgConcept CFG>
std::string print(const CFG& cfg) noexcept
{
    std::stringstream out;
    out << "digraph CFG {\n";
    int counter = 0;
    for (const auto& block : cfg.blocks())
    {
        out << "  Node_" << counter << R"([label=")";
        for (auto op : block.operations())
            out << print(toNode(op)) << R"(\n)";

        out << R"("])" << "\n";
        ++counter;
    }
    out << "\n";
    counter = 0;
    for (const auto& block : cfg.blocks())
    {
        for (auto next : block.successors())
            out << "  Node_" << counter << " -> " << "Node_" << next << "\n";

        ++counter;
    }
    out << "}\n";
    return std::move(out).str();
}

#endif // ANALYSIS_H