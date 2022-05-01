#include "include/cfg.h"

#include <algorithm>
#include <sstream>
#include <stack>
#include <cassert>

Node toNode(Operation op) noexcept
{
    return std::visit([](const auto* node) noexcept -> Node { return node; }, op);
}

int addAstNode(CFG& cfg, int currentBlock, Node currentNode)
{
    struct
    {
        int currentBlock;
        CFG& cfg;

        int newBlock() noexcept
        {
            cfg.blocks.emplace_back();
            return cfg.blocks.size() - 1;
        }

        int operator()(const Init* i) noexcept
        {
            cfg.blocks[currentBlock].operations.push_back(i);
            return currentBlock;
        }
        int operator()(const Translation* t) noexcept
        {
            cfg.blocks[currentBlock].operations.push_back(t);
            return currentBlock;
        }
        int operator()(const Rotation* r) noexcept
        {
            cfg.blocks[currentBlock].operations.push_back(r);
            return currentBlock;
        }
        int operator()(const Sequence* s) noexcept
        {
            for (auto node : s->nodes)
            {
                currentBlock = addAstNode(cfg, currentBlock, node);
            }
            return currentBlock;
        }
        int operator()(const Branch* b) noexcept
        {
            int lhsBlock = newBlock();
            int rhsBlock = newBlock();
            int branchPred = currentBlock;
            int lhsAfter = addAstNode(cfg, lhsBlock, b->lhs);
            int rhsAfter = addAstNode(cfg, rhsBlock, b->rhs);
            cfg.addEdge(branchPred, lhsBlock);
            cfg.addEdge(branchPred, rhsBlock);
            // TODO: can we do something smarter to avoid empty nodes with e.g., nested ors?
            int afterBranch = newBlock();
            cfg.addEdge(lhsAfter, afterBranch);
            cfg.addEdge(rhsAfter, afterBranch);
            return afterBranch;
        }
        int operator()(const Loop* l) noexcept
        {
            int bodyBegin = newBlock();
            cfg.addEdge(currentBlock, bodyBegin);
            int bodyEnd = addAstNode(cfg, bodyBegin, l->body);
            int afterBody = newBlock();
            cfg.addEdge(bodyEnd, bodyBegin);
            cfg.addEdge(bodyEnd, afterBody);
            return afterBody;
        }

    } processNode(currentBlock, cfg);
    
    return std::visit(processNode, currentNode);
}

CFG createCfg(Node root) noexcept
{
    CFG cfg;
    // Add start block.
    cfg.blocks.emplace_back();

    addAstNode(cfg, 0, root);
    return cfg;
}

CFG& CFG::addEdge(int from, int to)
{
    blocks[from].succs.push_back(to);
    blocks[to].preds.push_back(from);
    // TODO: add assertion that succs/preds has no duplicates.
    return *this;
}

std::string print(const CFG& cfg) noexcept
{
    std::stringstream out;
    out << "digraph CFG {\n";
    int counter = 0;
    for (const auto& block : cfg.blocks)
    {
        out << "  Node_" << counter << R"([label=")";
        for (auto op : block.operations)
        {
            out << print(toNode(op));
            out << R"(\n)";
        }
        out << R"("])" << "\n";
        ++counter;
    }
    out << "\n";
    counter = 0;
    for (const auto& block : cfg.blocks)
    {
        for (auto next : block.succs)
        {
            out << "  Node_" << counter << " -> " << "Node_" << next << "\n";
        }
        ++counter;
    }
    out << "}\n";
    return std::move(out).str();
}


RPOCompare::RPOCompare(const CFG& cfg)  : rpoOrder(cfg.blocks.size())
{
    std::vector<int> visitOrder;
    visitOrder.reserve(cfg.blocks.size());
    std::stack<int, std::vector<int>> stack;
    std::vector<bool> visited(cfg.blocks.size());
    std::stack<int, std::vector<int>> pending;
    stack.push(0);
    while(!stack.empty())
    {
        int current = stack.top();
        visited[current] = true;
        pending.push(-1);
        for(auto succ : cfg.blocks[current].succs)
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
  : cfg(cfg), comparator(cfg), worklist(comparator), queued(cfg.blocks.size(), false) {}

void RPOWorklist::enqueue(int node) noexcept
{
    if (queued[node])
        return;

    queued[node] = true;
    worklist.push(node);
}

void RPOWorklist::enqueueSuccessors(int node) noexcept
{
    for (int succ : cfg.blocks[node].succs)
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
