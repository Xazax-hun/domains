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

template<Domain D>
using AnnotatorFunc = Annotations (*)(const CFG& cfg, const std::vector<D>& result);

// TODO: covered area should be represented as a set of polygons instead of a vector.
template<Domain D>
using VisualizerFunc = std::vector<Polygon> (*)(const CFG& cfg, const std::vector<D>& result);

// Calculate the domain values for the end of each
// basic block using monotonic transfer functions.
// This function is doing forward analysis.
//
// The indices in the returned vector correspond to
// the IDs of the cfg nodes.
//
// While it would be possible to record the analysis state
// for each operation, it is often not required. Using the
// transfer functions one can recover the state for all
// operations and the user of the analysis often only needs
// the analysis state for a small subset of the operations.
//
// See `allAnnotationsFromAnalysisResults` how to recover
// per-operation analysis states.
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
            // TODO: consider switching the operands for transfer functions and
            //       introduce currying + caching. I.e., it would be possible
            //       to partially evaluate the transfer functions, see
            //       Futamura projections.
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

// Annotate the last operation of each CFG block with the analysis state at the end of the
// basic block.
template<Domain D>
Annotations blockEndAnnotationsFromAnalysisResults(const CFG& cfg, const std::vector<D>& result)
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

// Use the transfer functions to expand post-states to every operation.
template<Domain D, TransferFunction<D> F>
Annotations allAnnotationsFromAnalysisResults(const CFG& cfg, const std::vector<D>& result)
{
    Annotations anns;
    for (const auto& block : cfg.blocks)
    {
        D preState{ D::bottom() };
        for (auto pred : block.preds)
            preState = preState.merge(result[pred]);

        D postOperationState = preState;
        for (auto op : block.operations)
        {
            postOperationState = F{}(postOperationState, op);
            anns.postAnnotations[toNode(op)].emplace_back(postOperationState.toString());
        }
    }
    return anns;
}

template<Domain D, TransferFunction<D> F>
std::vector<Polygon> coveredAreaFromAnalysisResults(const CFG& cfg, const std::vector<D>& result)
{
    std::vector<Polygon> covered;
    for (const auto& block : cfg.blocks)
    {
        D preState{ D::bottom() };
        for (auto pred : block.preds)
            preState = preState.merge(result[pred]);

        D postOperationState = preState;
        for (auto op : block.operations)
        {
            postOperationState = F{}(postOperationState, op);
            for (const auto& poly : postOperationState.covers())
                covered.push_back(poly);
        }
    }
    return covered;
}

#endif // SOLVER_H