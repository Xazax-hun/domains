#include "include/cfg.h"

#include <sstream>

Node toNode(Operation op)
{
    struct {
        Node operator()(Init* i) const noexcept { return i; }
        Node operator()(Translation* t) const noexcept { return t; }
        Node operator()(Rotation* r) const noexcept { return r; }
    } converter;
    return std::visit(converter, op);
}

int addAstNode(CFG& cfg, int currentBlock, Node currentNode)
{
    struct
    {
        int currentBlock;
        CFG& cfg;

        int newBlock()
        {
            cfg.blocks.emplace_back();
            return cfg.blocks.size() - 1;
        }

        int operator()(Init* i) noexcept
        {
            cfg.blocks[currentBlock].operations.push_back(i);
            return currentBlock;
        }
        int operator()(Translation* t) noexcept
        {
            cfg.blocks[currentBlock].operations.push_back(t);
            return currentBlock;
        }
        int operator()(Rotation* r) noexcept
        {
            cfg.blocks[currentBlock].operations.push_back(r);
            return currentBlock;
        }
        int operator()(Sequence* s) noexcept
        {
            for (auto node : s->nodes)
            {
                currentBlock = addAstNode(cfg, currentBlock, node);
            }
            return currentBlock;
        }
        int operator()(Branch* b) noexcept
        {
            int lhsBlock = newBlock();
            int rhsBlock = newBlock();
            int branchPred = currentBlock;
            int lhsAfter = addAstNode(cfg, lhsBlock, b->lhs);
            int rhsAfter = addAstNode(cfg, rhsBlock, b->rhs);
            cfg.blocks[branchPred].nexts.push_back(lhsBlock);
            cfg.blocks[branchPred].nexts.push_back(rhsBlock);
            // TODO: can we do something smarter to avoid empty nodes with e.g., nested ors?
            int afterBranch = newBlock();
            cfg.blocks[lhsAfter].nexts.push_back(afterBranch);
            cfg.blocks[rhsAfter].nexts.push_back(afterBranch);
            return afterBranch;
        }
        int operator()(Loop* l) noexcept
        {
            int bodyBegin = newBlock();
            cfg.blocks[currentBlock].nexts.push_back(bodyBegin);
            int bodyEnd = addAstNode(cfg, bodyBegin, l->body);
            int afterBody = newBlock();
            cfg.blocks[bodyEnd].nexts.push_back(bodyBegin);
            cfg.blocks[bodyEnd].nexts.push_back(afterBody);
            return afterBody;
        }

    } processNode(currentBlock, cfg);
    
    return std::visit(processNode, currentNode);
}

CFG createCfg(Node node) noexcept
{
    CFG cfg;
    // Add start block.
    cfg.blocks.emplace_back();

    addAstNode(cfg, 0, node);
    return cfg;
}

std::string print(const CFG& cfg)
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
        for (auto next : block.nexts)
        {
            out << "  Node_" << counter << " -> " << "Node_" << next << "\n";
        }
        ++counter;
    }
    out << "}\n";
    return std::move(out).str();
}
