#ifndef SOLVER_H
#define SOLVER_H

#include <vector>

#include "include/dataflow/domains/domain.h"
#include "include/dataflow/transfer.h"
#include "include/cfg.h"

template<Domain D, CfgConcept CFG>
using AnalysisFunc = std::vector<D>(*)(const CFG&);

template<Domain D, CfgConcept CFG>
using AnnotatorFunc = Annotations (*)(const CFG& cfg, const std::vector<D>& result);

// TODO: covered area should be represented as a set of polygons instead of a vector.
template<Domain D, CfgConcept CFG>
using VisualizerFunc = std::vector<Polygon> (*)(const CFG& cfg, const std::vector<D>& result);

// Calculate the domain values for the end of each
// basic block using monotonic transfer functions.
// This function is doing forward analysis.
//
// The indices in the returned vector correspond to
// the IDs of the cfg nodes. If the analysis did not converge
// within NodeLimit * cfg.blocks.size(), an empty vector will
// be returned. NodeLimit of 0 means there is no upper bound
// on the number of iterations.
//
// While it would be possible to record the analysis state
// for each operation, it is often not required. Using the
// transfer functions one can recover the state for all
// operations and the user of the analysis often only needs
// the analysis state for a small subset of the operations.
//
// See `allAnnotationsFromAnalysisResults` how to recover
// per-operation analysis states.
template<Domain D, TransferFunction<D> F, CfgConcept CFG, unsigned NodeLimit = 10>
std::vector<D> solveMonotoneFramework(const CFG& cfg, F transfer)
{
    const size_t limit = NodeLimit * cfg.blocks().size();
    size_t processedNodes = 0;
    std::vector<D> postStates(cfg.blocks().size(), D::bottom());
    std::vector<bool> visited(cfg.blocks().size(), false);
    RPOWorklist w{ cfg };
    w.enqueue(0);
    while(!w.empty())
    {
        if (limit > 0 && processedNodes >= limit)
            return {};

        int currentBlock = w.dequeue();
        D preState{ D::bottom() };
        for (auto pred : cfg.blocks()[currentBlock].predecessors())
            preState = preState.join(postStates[pred]);

        D postState{ preState };
        // TODO: support per-block transfer functions. E.g., for bitvector
        //       style analyses.
        for (Operation op : cfg.blocks()[currentBlock].operations())
        {
            // TODO: consider currying for transfer functions for caching.
            //       I.e., it would be possible to partially evaluate the
            //       transfer functions, see Futamura projections.
            postState = transfer(op, postState);
        }
        ++processedNodes;
        // If the state did not change we do not need to propagate the changes.
        // If the first time we visit a node the transfer function produces bottom,
        // we do not want to terminate the analysis prematurely. Not every analysis
        // uses bottom to represent dead code.
        if (visited[currentBlock] && postStates[currentBlock] == postState)
            continue;
        
        visited[currentBlock] = true;
        postStates[currentBlock] = postState;
        w.enqueueSuccessors(currentBlock);
    }

    return postStates;
}

template<Domain D, TransferFunction<D> F, CfgConcept CFG, unsigned NodeLimit = 10>
std::vector<D> solveMonotoneFramework(const CFG& cfg)
{
    return solveMonotoneFramework<D, F, CFG, NodeLimit>(cfg, F{});
}

// Similar to solveMonotoneFramework, but always invoke the widen
// operation.
template<WidenableDomain D, TransferFunction<D> F, CfgConcept CFG, unsigned NodeLimit = 10>
std::vector<D> solveMonotoneFrameworkWithWidening(const CFG& cfg, F transfer)
{
    const size_t limit = NodeLimit * cfg.blocks().size();
    size_t processedNodes = 0;
    std::vector<D> preStates(cfg.blocks().size(), D::bottom());
    std::vector<D> postStates(cfg.blocks().size(), D::bottom());
    std::vector<bool> visited(cfg.blocks().size(), false);
    RPOWorklist w{ cfg };
    w.enqueue(0);
    while(!w.empty())
    {
        if (limit > 0 && processedNodes >= limit)
            return {};

        int currentBlock = w.dequeue();
        D newPreState{ D::bottom() };
        for (auto pred : cfg.blocks()[currentBlock].predecessors())
            newPreState = newPreState.join(postStates[pred]);

        preStates[currentBlock] = preStates[currentBlock].widen(newPreState);
        D postState{ preStates[currentBlock] };
        for (Operation op : cfg.blocks()[currentBlock].operations())
            postState = transfer(op, postState);

        ++processedNodes;
        // If the state did not change we do not need to
        // propagate the changes.
        if (visited[currentBlock] && postStates[currentBlock] == postState)
            continue;

        visited[currentBlock] = true;
        postStates[currentBlock] = postState;
        w.enqueueSuccessors(currentBlock);
    }

    return postStates;
}

template<WidenableDomain D, TransferFunction<D> F, CfgConcept CFG, unsigned NodeLimit = 10>
std::vector<D> solveMonotoneFrameworkWithWidening(const CFG& cfg)
{
    return solveMonotoneFrameworkWithWidening<D, F, CFG, NodeLimit>(cfg, F{});
}

// Annotate the last operation of each CFG block with the analysis state at the end of the
// basic block.
template<Domain D, CfgConcept CFG>
Annotations blockEndAnnotationsFromAnalysisResults(const CFG& cfg, const std::vector<D>& result)
{
    Annotations anns;
    int i = 0;
    for (const auto& block : cfg.blocks())
    {
        if (!block.operations().empty())
            anns.postAnnotations[toNode(block.operations().back())].emplace_back(result[i].toString());
        ++i;
    }
    return anns;
}

// Use the transfer functions to expand post-states to every operation.
template<Domain D, TransferFunction<D> F, CfgConcept CFG>
Annotations allAnnotationsFromAnalysisResults(const CFG& cfg, const std::vector<D>& result)
{
    Annotations anns;
    F transfer{};
    for (const auto& block : cfg.blocks())
    {
        D preState{ D::bottom() };
        for (auto pred : block.predecessors())
            preState = preState.join(result[pred]);

        D postOperationState = preState;
        for (auto op : block.operations())
        {
            postOperationState = transfer(op, postOperationState);
            if constexpr (std::is_same_v<CFG, ReverseCFG>)
                anns.preAnnotations[toNode(op)].emplace_back(postOperationState.toString());
            else
                anns.postAnnotations[toNode(op)].emplace_back(postOperationState.toString());
        }
    }
    return anns;
}

template<Domain D, TransferFunction<D> F, CfgConcept CFG>
std::vector<Polygon> coveredAreaFromAnalysisResults(const CFG& cfg, const std::vector<D>& result)
{
    std::vector<Polygon> covered;
    F transfer{};
    for (const auto& block : cfg.blocks())
    {
        D preState{ D::bottom() };
        for (auto pred : block.predecessors())
            preState = preState.join(result[pred]);

        D postOperationState = preState;
        for (auto op : block.operations())
        {
            postOperationState = transfer(op, postOperationState);
            for (const auto& poly : postOperationState.covers())
                covered.push_back(poly);
        }
    }
    return covered;
}

#endif // SOLVER_H