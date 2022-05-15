#ifndef SOLVER_H
#define SOLVER_H

#include <vector>

#include "include/dataflow/domains/domain.h"
#include "include/cfg.h"

#include <type_traits>

template<Domain D>
using AnalysisFunc = std::vector<D>(*)(const CFG&);

template<typename T, typename Dom>
concept TransferFunction = Domain<Dom> && 
    std::is_default_constructible_v<T> &&
    requires(T f, Dom d, Operation o)
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
    RPOWorklist w{ cfg };
    w.enqueue(0);
    while(!w.empty())
    {
        int currentBlock = w.dequeue();
        D preState{ D::bottom() };
        for (auto pred : cfg.blocks[currentBlock].preds)
        {
            preState = preState.merge(postStates[pred]);
        }
        D postState{ preState };
        for (Operation o : cfg.blocks[currentBlock].operations)
        {
            postState = F{}(postState, o);
        }
        // If the state did not change we do not need to
        // propagate the changes.
        if (postStates[currentBlock] == postState)
            continue;

        postStates[currentBlock] = postState;
        w.enqueueSuccessors(currentBlock);
    }

    return postStates;
}

// Similar to solveMonotoneFramework, but always invoke the widen
// operation.
template<WidenableDomain D, TransferFunction<D> F>
std::vector<D> solveMonotoneFrameworkWithWidening(const CFG& cfg)
{
    std::vector<D> preStates(cfg.blocks.size(), D::bottom());
    std::vector<D> postStates(cfg.blocks.size(), D::bottom());
    RPOWorklist w{ cfg };
    w.enqueue(0);
    while(!w.empty())
    {
        int currentBlock = w.dequeue();
        D newPreState{ D::bottom() };
        for (auto pred : cfg.blocks[currentBlock].preds)
        {
            newPreState = newPreState.merge(postStates[pred]);
        }
        preStates[currentBlock] = preStates[currentBlock].widen(newPreState);
        D postState{ preStates[currentBlock] };
        for (Operation o : cfg.blocks[currentBlock].operations)
        {
            postState = F{}(postState, o);
        }
        // If the state did not change we do not need to
        // propagate the changes.
        if (postStates[currentBlock] == postState)
            continue;

        postStates[currentBlock] = postState;
        w.enqueueSuccessors(currentBlock);
    }

    return postStates;
}

template<Domain D>
Annotations annotationsFromAnalysisResults(const std::vector<D>& result, const CFG& cfg)
{
    Annotations anns;
    int i = 0;
    for (const auto& block : cfg.blocks)
    {
        if (!block.operations.empty())
        {
            anns.postAnnotations[toNode(block.operations.back())].emplace_back(result[i].toString());
        }
        ++i;
    }
    return anns;
}

#endif // SOLVER_H