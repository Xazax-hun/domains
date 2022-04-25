#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <optional>

#include "include/lexer.h"
#include "include/ast.h"
#include "include/utils.h"

// Grammar:
// start -> seq
// seq -> command | seq ";" command | empty
// command -> INIT | TRANS | ROT | loop | branch
// loop -> "iter" "{" seq "}"
// branch -> "{" seq "}" "or" "{" seq "}"
class Parser
{
public:
    Parser(std::vector<Token> tokens,
           const DiagnosticEmitter& diag) noexcept : tokens(std::move(tokens)), diag(diag) {}

    // Reentrant. Invoking again will continue parsing with the tokens
    // added since the last invocation.
    std::optional<Node> parse();

    const ASTContext& getContext() const { return context; }

private:
    std::optional<Node> sequence(bool root = false);
    std::optional<Node> branch();
    std::optional<Node> loop();
    std::optional<Node> command();

    // Utilities.
    Token peek() const noexcept { return tokens[current]; }
    Token previous() const noexcept { return tokens[current - 1]; }
    bool isAtEnd() const noexcept
    {
        return peek().type == TokenType::END_OF_FILE;
    }

    bool check(TokenType type) const noexcept
    {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    template<typename... T>
    bool match(T... tokenTypes) noexcept
    {
        bool b = (check(tokenTypes) || ...);
        if (b)
            advance();
        return b;
    }

    Token advance() noexcept
    {
        if (!isAtEnd()) ++current;
        return previous();
    }

    std::optional<Token> consume(TokenType type, std::string_view message) noexcept
    {
        if (check(type))
            return advance();

        error(peek(), message);
        return std::nullopt;
    }

    void error(Token t, std::string_view message) noexcept;

    ASTContext context;
    std::vector<Token> tokens;
    unsigned current = 0;
    const DiagnosticEmitter& diag;
};

#endif // PARSER_H