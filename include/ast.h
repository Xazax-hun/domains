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
    Node lhs, rhs;
};

struct Loop
{
    Token kw;
    Node body;
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
        void operator()(const Init* i) const noexcept { delete i; }
        void operator()(const Translation* t) const noexcept { delete t; }
        void operator()(const Rotation* r) const noexcept { delete r; }
        void operator()(const Sequence* s) const noexcept { delete s; }
        void operator()(const Branch* b) const noexcept { delete b; }
        void operator()(const Loop* l) const noexcept { delete l; }
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

#endif // AST_H