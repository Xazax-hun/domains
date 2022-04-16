#ifndef LEXER_H
#define LEXER_H

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "include/utils.h"

enum class TokenType : unsigned char
{
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, SEMICOLON,

    // Literals.
    NUMBER,

    // Keywords.
    INIT, TRANSLATION, ROTATION, ITER, OR,

    END_OF_FILE
};

constexpr std::string_view tokenTypeToSourceName(TokenType type)
{
    using enum TokenType;
    switch(type)
    {
        case LEFT_PAREN: return "(";
        case RIGHT_PAREN: return ")";
        case LEFT_BRACE: return "{";
        case RIGHT_BRACE: return "}";
        case COMMA: return ",";
        case SEMICOLON: return ";";
        case NUMBER: return "NUMBER";
        case INIT: return "init";
        case TRANSLATION: return "translation";
        case ROTATION: return "rotation";
        case ITER: return "iter";
        case OR: return "or";
        case END_OF_FILE: return "END_OF_FILE";
    }
    assert(false && "Unhandled token type");
    return "";
}

struct Token
{
    TokenType type;

    // TODO: add better location info:
    //       location should be an index into
    //       a table that has line number,
    //       column number and file path.
    unsigned line; 

    // The value of number literals.
    std::optional<int> value;

    Token(TokenType type, int line, std::optional<int> value = {}) noexcept :
        type(type), line(line), value(std::move(value)) {}
};

std::string print(Token t) noexcept;

class Lexer
{
public:
    Lexer(std::string source, const DiagnosticEmitter& diag) noexcept
        : source(std::move(source)), diag(diag) {}

    std::vector<Token> lexAll() noexcept;

    int getBracketBalance() const noexcept { return bracketBalance; }

private:
    std::optional<Token> lex() noexcept;
    std::optional<Token> lexNumber() noexcept;
    std::optional<Token> lexKeyword() noexcept;
    bool isAtEnd() const noexcept { return static_cast<unsigned>(current) >= source.length(); }
    char advance() noexcept { return source[current++]; }
    char peek() const noexcept;
    char peekNext() const noexcept;
    bool match(char expected) noexcept;

    std::string source;
    const DiagnosticEmitter& diag;
    int start = 0;
    int current = 0;
    int line = 1;
    int bracketBalance = 0;
    bool hasError = false; // TODO: get rid of this.
};

#endif // LEXER_H
