#ifndef UTILS_H
#define UTILS_H

#include <iosfwd>
#include <string_view>

class DiagnosticEmitter
{
public:
    DiagnosticEmitter(std::ostream& out, std::ostream& err) noexcept
        : out(out), err(err) {}

    void error(int line, std::string_view message) const noexcept;
    void report(int line, std::string_view where, std::string_view message) const noexcept;

    std::ostream& getOutput() const { return out; }
private:
    std::ostream& out;
    std::ostream& err;
};

template<typename T>
struct Finally {
    T t;
    ~Finally() { t(); }
};

struct Vec2 { int x, y; };

#endif // UTILS_H