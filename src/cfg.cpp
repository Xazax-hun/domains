#include "include/cfg.h"


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
