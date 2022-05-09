#ifndef AST_H
#define AST_H

#include <variant>
#include <vector>
#include <unordered_map>

#include "include/lexer.h"

struct Init;
struct Translation;
struct Rotation;
struct Sequence;
struct Branch;
struct Loop;

// TODO: add language elements to express assertions about the state
//       that can be checked statically and dynamically.
using Node = std::variant<const Init*, const Translation*, const Rotation*,
                          const Sequence*, const Branch*, const Loop*>;

struct Init
{
    Token kw;
    Token topX, topY;
    Token width, height;
};

struct Translation
{
    Token kw;
    Token x, y;
};

struct Rotation
{
    Token kw;
    Token x, y, deg;
};

struct Sequence
{
    std::vector<Node> nodes;
};

struct Branch
{
    Token kw;
    const Sequence* lhs;
    const Sequence* rhs;
};

struct Loop
{
    Token kw;
    const Sequence* body;
};

class ASTContext
{
public:
    ASTContext() = default;
    ASTContext(ASTContext&&) = default;
    ASTContext& operator=(ASTContext&&) = default;
    ASTContext(const ASTContext&) = delete;
    ASTContext& operator=(const ASTContext&) = delete;

    template<typename T, typename... Args>
    const T* make(Args&&... args)
    {
        const T* ret = new T(std::forward<Args&&>(args)...);
        nodes.push_back(ret);
        return ret;
    }

    ~ASTContext()
    {
        for (auto node : nodes)
        {
            std::visit(nodeDeleter, node);
        }
    }

private:
    std::vector<Node> nodes;

    struct {
        void operator()(const auto* p) const noexcept { delete p; }
    } nodeDeleter;
};


// Utility to attach information before and after AST nodes that
// can be rendered by pretty-printing the AST. This can be useful
// to visualize dataflow analysis results.
struct Annotations
{
    using AnnotationMap = std::unordered_map<Node, std::vector<std::string>>;
    AnnotationMap preAnnotations; // Rendered before the node.
    AnnotationMap postAnnotations; // Rendered after the node.
};

std::string print(Node n, const Annotations& anns = {}) noexcept;

// TODO: add recursive AST visitor.

#endif // AST_H