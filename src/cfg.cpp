#include "include/cfg.h"

#include <algorithm>
#include <sstream>
#include <stack>
#include <cassert>

int CFG::addAstNode(int currentBlock, Node currentNode)
{
    struct
    {
        int currentBlock;
        CFG& cfg;

        int operator()(const Init* i) noexcept
        {
            cfg.basicBlocks[currentBlock].ops.emplace_back(i);
            return currentBlock;
        }
        int operator()(const Translation* t) noexcept
        {
            cfg.basicBlocks[currentBlock].ops.emplace_back(t);
            return currentBlock;
        }
        int operator()(const Rotation* r) noexcept
        {
            cfg.basicBlocks[currentBlock].ops.emplace_back(r);
            return currentBlock;
        }
        int operator()(const Sequence* s) noexcept
        {
            for (auto node : s->nodes)
            {
                currentBlock = cfg.addAstNode(currentBlock, node);
            }
            return currentBlock;
        }
        int operator()(const Branch* b) noexcept
        {
            int lhsBlock = cfg.newBlock();
            int rhsBlock = cfg.newBlock();
            int branchPred = currentBlock;
            int lhsAfter = cfg.addAstNode(lhsBlock, b->lhs);
            int rhsAfter = cfg.addAstNode(rhsBlock, b->rhs);
            cfg.addEdge(branchPred, lhsBlock);
            cfg.addEdge(branchPred, rhsBlock);
            // TODO: can we do something smarter to avoid empty nodes with e.g., nested ors?
            int afterBranch = cfg.newBlock();
            cfg.addEdge(lhsAfter, afterBranch);
            cfg.addEdge(rhsAfter, afterBranch);
            return afterBranch;
        }
        int operator()(const Loop* l) noexcept
        {
            int bodyBegin = cfg.newBlock();
            cfg.addEdge(currentBlock, bodyBegin);
            int bodyEnd = cfg.addAstNode(bodyBegin, l->body);
            int afterBody = cfg.newBlock();
            cfg.addEdge(bodyEnd, bodyBegin);
            cfg.addEdge(bodyEnd, afterBody);
            return afterBody;
        }

    } processNode(currentBlock, *this);
    
    return std::visit(processNode, currentNode);
}

CFG CFG::createCfg(Node root) noexcept
{
    CFG cfg;
    // Add start block.
    cfg.basicBlocks.emplace_back();

    cfg.addAstNode(0, root);
    return cfg;
}

CFG& CFG::addEdge(int from, int to)
{
    basicBlocks[from].succs.push_back(to);
    basicBlocks[to].preds.push_back(from);
    // TODO: add assertion that succs/preds has no duplicates.
    return *this;
}

int CFG::newBlock()
{
    basicBlocks.emplace_back();
    return basicBlocks.size() - 1;
}

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


RPOCompare::RPOCompare(const CFG& cfg)  : rpoOrder(cfg.blocks().size())
{
    std::vector<int> visitOrder;
    visitOrder.reserve(cfg.blocks().size());
    std::stack<int, std::vector<int>> stack;
    std::vector<bool> visited(cfg.blocks().size());
    std::stack<int, std::vector<int>> pending;
    stack.push(0);
    while(!stack.empty())
    {
        int current = stack.top();
        visited[current] = true;
        pending.push(-1);
        for(auto succ : cfg.blocks()[current].successors())
        {
            if (!visited[succ])
                pending.push(succ);
        }
        int next = pending.top();
        pending.pop();
        if (next == -1)
        {
            stack.pop();
            visitOrder.push_back(current);
            continue;
        }
        stack.push(next);
    }
    std::reverse(visitOrder.begin(), visitOrder.end());
    int counter = 0;
    for (int node : visitOrder)
        rpoOrder[node] = counter++;
}

RPOWorklist::RPOWorklist(const CFG& cfg)
  : cfg(cfg), comparator(cfg), worklist(comparator), queued(cfg.blocks().size(), false) {}

void RPOWorklist::enqueue(int node) noexcept
{
    if (queued[node])
        return;

    queued[node] = true;
    worklist.push(node);
}

void RPOWorklist::enqueueSuccessors(int node) noexcept
{
    for (int succ : cfg.blocks()[node].successors())
        enqueue(succ);
}

int RPOWorklist::dequeue() noexcept
{
    assert(!worklist.empty());
    int node = worklist.top();
    worklist.pop();
    queued[node] = false;
    return node;
}
