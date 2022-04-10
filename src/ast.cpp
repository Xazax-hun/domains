#include "include/ast.h"

#include <iostream>
#include <fmt/format.h>

void print(int indent, Node n) noexcept;

namespace
{
    std::string indentString(int n)
    {
        return std::string(n, ' ');
    }

    struct NodePrinter {
        void operator()(Init* i) noexcept
        {
            out << indentString(indent);
            out << fmt::format("init({}, {}, {}, {})", *i->topX.value, *i->topY.value, *i->width.value, *i->height.value);
        }
        void operator()(Translation* t) noexcept
        {
            out << indentString(indent);
            out << fmt::format("translation({}, {})", *t->x.value, *t->y.value);
        }
        void operator()(Rotation* r) noexcept
        {
            out << indentString(indent);
            out << fmt::format("rotation({}, {}, {})", *r->x.value, *r->y.value, *r->deg.value);
        }
        void operator()(Sequence* s) noexcept
        {
            int i = 0;
            const int size = s->nodes.size();
            for (auto n : s->nodes)
            {
                print(indent, n);
                if (i++ < size - 1)
                    out << ";\n";
            }
        }
        void operator()(Branch* b) noexcept
        {

            out << indentString(indent) << "{\n";
            print(indent + 2, b->lhs);
            out << "\n" << indentString(indent) << "} or {\n";
            print(indent + 2, b->rhs);
            out << "\n" << indentString(indent) << "}";
        }
        void operator()(Loop* l) noexcept
        {
            out << indentString(indent) << "iter {\n";
            print(indent + 2, l->body);
            out << indentString(indent) << "\n}";
        }

        int indent;
        std::ostream& out;
    };
} // anonymous namespace

void print(Node n) noexcept
{
    print(0, n);
}


void print(int indent, Node n) noexcept
{
    NodePrinter nodePrinter{indent, std::cout};
    std::visit(nodePrinter, n);
}


