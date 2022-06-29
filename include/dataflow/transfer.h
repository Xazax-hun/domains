#ifndef TRANSFER_H
#define TRANSFER_H

#include "include/dataflow/domains/domain.h"
#include "include/cfg.h"

// TODO: Abstract over the Operation type?
template<typename T, typename Dom>
concept TransferFunction = Domain<Dom> && 
    requires(T f, Operation op, Dom d)
{
    { f(op, d) } -> std::same_as<Dom>;
};

template<Domain D, TransferFunction<D> TransferFunc, typename Logger>
struct TransferTracer
{
    D operator()(Operation op, D input) {
       D result = wrapped(op, input);
       logger.log(op, input, result);
       return result;
    }

    TransferFunc wrapped;
    Logger logger;
};

// TODO: more transfer function wrappers:
//  * Memoization wrapper
//  * Could widening be a wrapper?
//  * Chain transfer functions, an option to reorganize code
//    by separating one big transfer function into many smaller ones
//  * Combine transfer functions for reduced product domains

#endif // TRANSFER_H