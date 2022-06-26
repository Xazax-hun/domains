#include "include/ast.h"

#include <sstream>
#include <fmt/format.h>

void print(int indent, Node n, std::ostream& output, const Annotations& anns) noexcept;

namespace
{
    std::string indentString(int n)
    {
        return std::string(n, ' ');
    }

    template<bool Pre = false>
    void renderAnnotations(std::ostream& out, Node n, const Annotations::AnnotationMap& map) noexcept
    {
        if (auto it = map.find(n); it != map.end())
        {
            if (!it->second.empty())
            {
                out << (Pre ? "" : " ") << "/*";
                for (const auto& annotation : it->second)
                    out << " " << annotation;

                out << " */" << (Pre ? " " : "");
            }
        }
    }

    void renderPreAnnotations(std::ostream& out, Node n, const Annotations& anns) noexcept
    {
        renderAnnotations</*Pre =*/true>(out, n, anns.preAnnotations);
    }

    void renderPostAnnotations(std::ostream& out, Node n, const Annotations& anns) noexcept
    {
        renderAnnotations(out, n, anns.postAnnotations);
    }

    struct NodePrinter {
        void operator()(const Init* i) const noexcept
        {
            out << fmt::format("init({}, {}, {}, {})", *i->topX.value, *i->topY.value, *i->width.value, *i->height.value);
        }
        void operator()(const Translation* t) const noexcept
        {
            out << fmt::format("translation({}, {})", *t->x.value, *t->y.value);
        }
        void operator()(const Rotation* r) const noexcept
        {
            out << fmt::format("rotation({}, {}, {})", *r->x.value, *r->y.value, *r->deg.value);
        }
        void operator()(const Sequence* s) const noexcept
        {
            int i = 0;
            const int size = s->nodes.size();
            for (auto n : s->nodes)
            {
                print(indent, n, out, anns);
                if (i++ < size - 1)
                    out << ";\n";
            }
        }
        void operator()(const Branch* b) const noexcept
        {
            out << "{\n";
            print(indent + 2, b->lhs, out, anns);
            out << "\n" << indentString(indent) << "} or {\n";
            print(indent + 2, b->rhs, out, anns);
            out << "\n" << indentString(indent) << "}";
        }
        void operator()(const Loop* l) const noexcept
        {
            out << "iter {\n";
            print(indent + 2, l->body, out, anns);
            out << "\n" << indentString(indent) << "}";
        }
        int indent;
        std::ostream& out;
        const Annotations& anns;
    };
} // anonymous namespace

std::string print(Node n, const Annotations& anns) noexcept
{
    std::stringstream output;
    print(0, n, output, anns);
    return std::move(output).str();
}


void print(int indent, Node n, std::ostream& output, const Annotations& anns) noexcept
{
    if (!std::holds_alternative<const Sequence*>(n))
        output << indentString(indent);
    renderPreAnnotations(output, n, anns);
    NodePrinter nodePrinter{indent, output, anns};
    std::visit(nodePrinter, n);
    renderPostAnnotations(output, n, anns);
}


