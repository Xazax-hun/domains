#include "include/parser.h"

#include <fmt/format.h>

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define BIND(var, x)  BIND_IMPL(var, x, CONCAT(__val, __COUNTER__))
#define BIND_IMPL(var, x, y)       \
  auto y = (x);                    \
  if (!y) return std::nullopt;     \
  auto var = *y

#define MUST_SUCCEED(x)  MUST_SUCCEED_IMPL(x, CONCAT(__val, __COUNTER__))
#define MUST_SUCCEED_IMPL(x, y)       \
  auto y = (x);                       \
  if (!y) return std::nullopt; 


std::optional<const Sequence*> Parser::parse()
{
    auto result = sequence(true);
    if (!isAtEnd())
    {
        error(peek(), "end of file expected.");
        return {};
    }
    return result;
}

std::optional<const Sequence*> Parser::sequence(bool root)
{
    if (root && !check(TokenType::INIT))
    {
        error(peek(), "'init' expected at the beginning of the program.");
        return {};
    }

    std::vector<Node> commands;
    do
    {
        BIND(com, command());
        commands.push_back(com);
    } while (match(TokenType::SEMICOLON));

    const auto* seq = context.make<Sequence>(std::move(commands));
    return seq;
}

std::optional<Node> Parser::command()
{
    if (match(TokenType::INIT))
    {
        Token kw = previous();
        MUST_SUCCEED(consume(TokenType::LEFT_PAREN));
        BIND(topX, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::COMMA))
        BIND(topY, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::COMMA))
        BIND(width, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::COMMA))
        BIND(height, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::RIGHT_PAREN));

        if (*width.value < 0)
        {
            error(kw, "the width of the initial area cannot be negative.");
            return {};
        }
        if (*height.value < 0)
        {
            error(kw, "the height of the initial area cannot be negative.");
            return {};
        }

        return context.make<Init>(kw, topX, topY, width, height);
    }
    if (match(TokenType::TRANSLATION))
    {
        Token kw = previous();
        MUST_SUCCEED(consume(TokenType::LEFT_PAREN));
        BIND(x, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::COMMA))
        BIND(y, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::RIGHT_PAREN));

        return context.make<Translation>(kw, x, y);
    }
    if (match(TokenType::ROTATION))
    {
        Token kw = previous();
        MUST_SUCCEED(consume(TokenType::LEFT_PAREN));
        BIND(x, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::COMMA))
        BIND(y, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::COMMA))
        BIND(deg, consume(TokenType::NUMBER, "a number expected."));
        MUST_SUCCEED(consume(TokenType::RIGHT_PAREN));

        return context.make<Rotation>(kw, x, y, deg);
    }
    if (match(TokenType::ITER))
        return loop();
    if (match(TokenType::LEFT_BRACE))
        return branch();

    if (isAtEnd() || check(TokenType::RIGHT_BRACE))
        error(peek(), "redundant semicolon?");

    return {};
}

std::optional<const Branch*> Parser::branch()
{
    // Exception to allow empty sequence in alternatives.
    const Sequence* lhs = nullptr;
    if (check(TokenType::RIGHT_BRACE))
        lhs = context.make<Sequence>(std::vector<Node>{});
    else
    {
        BIND(lhs_candidate, sequence());
        lhs = lhs_candidate;
    }

    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE));
    BIND(kw, consume(TokenType::OR));
    MUST_SUCCEED(consume(TokenType::LEFT_BRACE));

    const Sequence* rhs = nullptr;
    if (check(TokenType::RIGHT_BRACE))
        rhs = context.make<Sequence>(std::vector<Node>{});
    else
    {
        BIND(rhs_candidate, sequence());
        rhs = rhs_candidate;
    }

    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE));

    assert(lhs && rhs);
    if (lhs->nodes.empty() && rhs->nodes.empty())
    {
        error(kw, "at most one alternative can be empty.");
        return {};
    }

    return context.make<Branch>(kw, lhs, rhs);
}

std::optional<const Loop*> Parser::loop()
{
    Token kw = previous();
    MUST_SUCCEED(consume(TokenType::LEFT_BRACE));

    if (match(TokenType::RIGHT_BRACE))
    {
        error(kw, "the body of 'iter' must not be empty.");
        return {};
    }

    BIND(body, sequence());
    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE));

    return context.make<Loop>(kw, body);
}


void Parser::error(Token t, std::string_view message) const noexcept
{
    if (t.type == TokenType::END_OF_FILE)
        diag.report(t.line, "at end of file", message);
    else
        diag.report(t.line, fmt::format("at '{}'", print(t)), message);
}
