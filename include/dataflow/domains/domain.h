#ifndef DOMAIN_H
#define DOMAIN_H

#include <concepts>
#include <string>
#include <string_view>

template<typename T>
concept Domain = requires(T a)
{
    // Required to be an equivalence relation.
    { a == a } -> std::convertible_to<bool>;
    // Required to be a partial order.
    { a <= a } -> std::convertible_to<bool>;
    // Required to be the smallest element according to the order above.
    { T::bottom() } -> std::same_as<T>;
    // Requirements:
    // * a.merge(a) == a
    // * a.merge(b) == b.merge(a)
    // * a.merge(b) >= a
    // * a.merge(b) >= b
    // * top.merge(b) == top
    // * bottom.merge(b) == b
    { a.merge(a) } -> std::same_as<T>;
    { a.toString() } -> std::convertible_to<std::string_view>;
};

template<typename T>
concept WidenableDomain = Domain<T> &&
    requires(T a)
{
    // Requirements:
    // * bottom.widen(a) == a
    // * a.widen(a) == a
    // * b.widen(a) == b if a <= b
    { a.widen(a) } -> std::same_as<T>; 
};

#endif // SOLVER_H
