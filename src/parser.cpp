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
        error(peek(), "end of file expected");
        return {};
    }
    return result;
}

std::optional<const Sequence*> Parser::sequence(bool root)
{
    if (root && !check(TokenType::INIT))
    {
        error(peek(), "'init' expected at the beginning of the program");
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
        MUST_SUCCEED(consume(TokenType::LEFT_PAREN, "( expected"));
        BIND(topX, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::COMMA, ", expected"))
        BIND(topY, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::COMMA, ", expected"))
        BIND(width, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::COMMA, ", expected"))
        BIND(height, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::RIGHT_PAREN, ") expected"));

        if (*width.value < 0)
        {
            error(kw, "the width of the initial area cannot be negative");
            return {};
        }
        if (*height.value < 0)
        {
            error(kw, "the height of the initial area cannot be negative");
            return {};
        }

        return context.make<Init>(kw, topX, topY, width, height);
    }
    if (match(TokenType::TRANSLATION))
    {
        Token kw = previous();
        MUST_SUCCEED(consume(TokenType::LEFT_PAREN, "( expected"));
        BIND(x, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::COMMA, ", expected"))
        BIND(y, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::RIGHT_PAREN, ") expected"));

        return context.make<Translation>(kw, x, y);
    }
    if (match(TokenType::ROTATION))
    {
        Token kw = previous();
        MUST_SUCCEED(consume(TokenType::LEFT_PAREN, "( expected"));
        BIND(x, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::COMMA, ", expected"))
        BIND(y, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::COMMA, ", expected"))
        BIND(deg, consume(TokenType::NUMBER, "number expected"));
        MUST_SUCCEED(consume(TokenType::RIGHT_PAREN, ") expected"));

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
    if (!check(TokenType::RIGHT_BRACE))
    {
        BIND(lhs_candidate, sequence());
        lhs = lhs_candidate;
    }
    else
        lhs = context.make<Sequence>(std::vector<Node>{});

    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE, "} expected"));
    BIND(kw, consume(TokenType::OR, "or expected"));
    MUST_SUCCEED(consume(TokenType::LEFT_BRACE, "{ expected"));

    const Sequence* rhs = nullptr;
    if (!check(TokenType::RIGHT_BRACE))
    {
        BIND(rhs_candidate, sequence());
        rhs = rhs_candidate;
    }
    else
        rhs = context.make<Sequence>(std::vector<Node>{});

    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE, "} expected"));

    assert(lhs && rhs);
    if (lhs->nodes.empty() && rhs->nodes.empty())
    {
        error(kw, "at most one alternative can be empty");
        return {};
    }

    return context.make<Branch>(kw, lhs, rhs);
}

std::optional<const Loop*> Parser::loop()
{
    Token kw = previous();
    MUST_SUCCEED(consume(TokenType::LEFT_BRACE, "{ expected"));

    if (match(TokenType::RIGHT_BRACE))
    {
        error(kw, "the body of 'iter' must not be empty");
        return {};
    }

    BIND(body, sequence());
    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE, "} expected"));

    return context.make<Loop>(kw, body);
}


void Parser::error(Token t, std::string_view message) const noexcept
{
    if (t.type == TokenType::END_OF_FILE)
        diag.report(t.line, "at end of file", message);
    else
        diag.report(t.line, fmt::format("at '{}'", print(t)), message);
}
