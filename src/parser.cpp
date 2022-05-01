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
    std::vector<Node> commands;
    if (root && peek().type != TokenType::INIT)
    {
        error(peek(), "'init' expected at the beginning of the program");
        return {};
    }
    while (auto com = command())
    {
        commands.push_back(*com);
        if (!match(TokenType::SEMICOLON))
            break;
    }

    auto seq = context.make<Sequence>(std::move(commands));
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
    {
        return loop();
    }
    if (match(TokenType::LEFT_BRACE))
    {
        return branch();
    }

    return {};
}

std::optional<const Branch*> Parser::branch()
{
    BIND(lhs, sequence());
    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE, "} expected"));
    BIND(kw, consume(TokenType::OR, "number expected"));
    MUST_SUCCEED(consume(TokenType::LEFT_BRACE, "{ expected"));
    BIND(rhs, sequence());
    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE, "} expected"));

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
    BIND(body, sequence());
    MUST_SUCCEED(consume(TokenType::RIGHT_BRACE, "} expected"));

    if (body->nodes.empty())
    {
        error(kw, "the body of 'iter' must not be empty");
        return {};
    }

    return context.make<Loop>(kw, body);
}


void Parser::error(Token t, std::string_view message) noexcept
{
    if (t.type == TokenType::END_OF_FILE)
    {
        diag.report(t.line, "at end of file", message);
    }
    else
    {
        diag.report(t.line, fmt::format("at '{}'", print(t)), message);
    }
}
