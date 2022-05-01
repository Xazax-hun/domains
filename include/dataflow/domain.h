#ifndef DOMAIN_H
#define DOMAIN_H

#include <concepts>
#include <string>

template<typename T>
concept Domain = requires(T a)
{
    { a == a } -> std::convertible_to<bool>;
    { a <= a } -> std::convertible_to<bool>;
    { T::bottom() } -> std::same_as<T>;
    { a.merge(a) } -> std::same_as<T>;
    { a.toString() } -> std::convertible_to<std::string_view>;
};

#endif // SOLVER_H