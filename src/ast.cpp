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

    struct NodePrinter {
        void operator()(Init* i) noexcept
        {
            renderPreAnnotations(i);
            out << indentString(indent);
            out << fmt::format("init({}, {}, {}, {})", *i->topX.value, *i->topY.value, *i->width.value, *i->height.value);
            renderPostAnnotations(i);
        }
        void operator()(Translation* t) noexcept
        {
            renderPreAnnotations(t);
            out << indentString(indent);
            out << fmt::format("translation({}, {})", *t->x.value, *t->y.value);
            renderPostAnnotations(t);
        }
        void operator()(Rotation* r) noexcept
        {
            renderPreAnnotations(r);
            out << indentString(indent);
            out << fmt::format("rotation({}, {}, {})", *r->x.value, *r->y.value, *r->deg.value);
            renderPostAnnotations(r);
        }
        void operator()(Sequence* s) noexcept
        {
            renderPreAnnotations(s);
            int i = 0;
            const int size = s->nodes.size();
            for (auto n : s->nodes)
            {
                print(indent, n, out, anns);
                if (i++ < size - 1)
                    out << ";\n";
            }
            renderPostAnnotations(s);
        }
        void operator()(Branch* b) noexcept
        {
            renderPreAnnotations(b);
            out << indentString(indent) << "{\n";
            print(indent + 2, b->lhs, out, anns);
            out << "\n" << indentString(indent) << "} or {\n";
            print(indent + 2, b->rhs, out, anns);
            out << "\n" << indentString(indent) << "}";
            renderPostAnnotations(b);
        }
        void operator()(Loop* l) noexcept
        {
            renderPreAnnotations(l);
            out << indentString(indent) << "iter {\n";
            print(indent + 2, l->body, out, anns);
            out << "\n" << indentString(indent) << "}";
            renderPostAnnotations(l);
        }
        void renderPreAnnotations(Node n) const
        {
            renderAnnotations(n, anns.preAnnotations);
        }
        void renderPostAnnotations(Node n) const
        {
            renderAnnotations(n, anns.postAnnotations);
        }
        void renderAnnotations(Node n, const Annotations::AnnotationMap& map) const
        {
            if (auto it = map.find(n); it != map.end())
            {
                if (!it->second.empty())
                {
                    out << " /*";
                    for (const auto& annotation : it->second)
                    {
                        out << " " << annotation;
                    }
                    out << " */";
                }
            }
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
    NodePrinter nodePrinter{indent, output, anns};
    std::visit(nodePrinter, n);
}


