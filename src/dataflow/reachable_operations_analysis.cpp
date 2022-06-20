#include "include/dataflow/analyses/reachable_operations_analysis.h"

#include "include/dataflow/solver.h"

namespace
{
struct TransferOperation
{
    StringSetDomain operator()(const Init*) const
    {
        return preState.insert("Init");
    }

    StringSetDomain operator()(const Translation*) const
    {
        return preState.insert("Translation");
    }

    StringSetDomain operator()(const Rotation*) const
    {
        return preState.insert("Rotation");
    }

    StringSetDomain preState;
};

struct ReachableOperationsTransfer
{
    StringSetDomain operator()(Operation op, StringSetDomain preState) const
    {
        return std::visit(TransferOperation{preState}, op);
    }
};
} // anonymous namespace

std::vector<StringSetDomain> getPastOperationsAnalysis(const CFG& cfg)
{
    return solveMonotoneFramework<StringSetDomain, ReachableOperationsTransfer>(cfg);
}

std::vector<StringSetDomain> getFutureOperationsAnalysis(const ReverseCFG& cfg)
{
    return solveMonotoneFramework<StringSetDomain, ReachableOperationsTransfer>(cfg);
}

Annotations pastOperationsAnalysisToOperationAnnotations(const CFG& cfg,
                                                         const std::vector<StringSetDomain>& results)
{
    return allAnnotationsFromAnalysisResults<StringSetDomain, ReachableOperationsTransfer>(cfg, results);
}

Annotations futureOperationsAnalysisToOperationAnnotations(const ReverseCFG& cfg,
                                                           const std::vector<StringSetDomain>& results)
{
    return allAnnotationsFromAnalysisResults<StringSetDomain, ReachableOperationsTransfer>(cfg, results);
}
