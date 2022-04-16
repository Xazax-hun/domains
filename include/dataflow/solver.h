#ifndef SOLVER_H
#define SOLVER_H

#include <vector>

#include "include/dataflow/domain.h"
#include "include/cfg.h"

template<typename T, typename Dom>
concept TransferFunction = Domain<Dom> && requires(T f, Dom d, Operation o)
{
    { f(d, o) } -> std::same_as<Dom>;
};

// Calculate the domain values for the end of each
// basic block using monotonic transfer functions.
// This function is doing forward analysis.
//
// The indices in the returned vector correspond to
// the IDs of the cfg nodes.
template<Domain D, TransferFunction<D> F>
std::vector<D> solveMonotoneFramework(const CFG& cfg)
{
    std::vector<D> postStates(cfg.blocks.size(), D::bottom());
    RPOWorklist w(cfg);
    w.enqueue(0);
    while(!w.empty())
    {
        int currentBlock = w.dequeue();
        D preState = D::bottom();
        for (auto pred : cfg.blocks[currentBlock].preds)
        {
            preState = preState.merge(postStates[pred]);
        }
        D postState = preState;
        for (Operation o : cfg.blocks[currentBlock].operations)
        {
            postState = F{}(postState, o);
        }
        // If the state did not chage we do not need to
        // propagate the changes.
        if (postStates[currentBlock] == postState)
            continue;

        postStates[currentBlock] = postState;
        w.enqueueSuccessors(currentBlock);
    }

    return postStates;
}

#endif // SOLVER_H