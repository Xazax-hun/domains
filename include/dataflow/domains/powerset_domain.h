#ifndef POWERSET_DOMAIN_H
#define POWERSET_DOMAIN_H

#include <fmt/format.h>

#include <set>
#include <initializer_list>

#include "include/dataflow/domains/domain.h"

template<typename Element>
struct PowersetDomain
{
    PowersetDomain() = default;
    explicit PowersetDomain(std::initializer_list<Element> e) : data{e} {}
    static PowersetDomain bottom() { return {}; }

    void insert(const Element& element)
    {
        data.insert(element);
    }

    bool operator==(const PowersetDomain& other) const
    {
        return data == other.data;
    }

    bool operator<=(const PowersetDomain& other) const
    {
        return std::includes(other.data.begin(), other.data.end(),
                             data.begin(), data.end());
    }

    PowersetDomain merge(PowersetDomain other) const
    {
        other.data.insert(data.begin(), data.end());
        return other;
    }

    std::string toString() const
    {
        std::string result{"{"};
        for (const auto& e : data)
        {
            if constexpr (requires { { std::declval<Element>().toString() } -> std::convertible_to<std::string_view>; })
                result += e.toString() + ", ";
            else
                result += fmt::format("{}", e) + ", ";
        }

        if (result.size() > 1)
        {
            result.pop_back();
            result.pop_back();
        }
        result += "}";
        return result;
    }

    std::vector<Polygon> covers() const { return {}; }
private:
    std::set<Element> data;
};

#endif // POWERSET_DOMAIN_H