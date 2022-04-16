#include "include/lexer.h"

#include <fmt/format.h>

#include <cstdlib>
#include <unordered_map>

using enum TokenType;

std::string print(Token t) noexcept
{
    switch (t.type)
    {
    case NUMBER:
        return std::to_string(*t.value);
    
    default:
        return std::string(tokenTypeToSourceName(t.type));
    }
}

std::vector<Token> Lexer::lexAll() noexcept
{
    std::vector<Token> result;

    while (!isAtEnd())
    {
        if (auto maybeToken = lex(); maybeToken.has_value())
            result.push_back(*maybeToken);
        else if (hasError)
            return {};
    }

    result.emplace_back(END_OF_FILE, line);
    return result;
}

std::optional<Token> Lexer::lex() noexcept
{
    while (true)
    {
        start = current;
        char c = advance();
        switch (c)
        {
        //  Unambiguous single characters tokens.
        case '(':
            ++bracketBalance;
            return Token(LEFT_PAREN, line);
        case ')':
            --bracketBalance;
            return Token(RIGHT_PAREN, line);
        case '{':
            ++bracketBalance;
            return Token(LEFT_BRACE, line);
        case '}':
            --bracketBalance;
            return Token(RIGHT_BRACE, line);

        case ',': return Token(COMMA, line);
        case ';': return Token(SEMICOLON, line);

        // Whitespace.
        case '\n':
            line++;
            [[fallthrough]];
        case ' ':
        case '\r':
        case '\t':
            break;

        // Comments
        case '/':
            if (match('/'))
            {
                // Skip to end of line for comment.
                while (peek() != '\n' && !isAtEnd())
                    advance();
                break;
            }
            diag.error(line, fmt::format("Unexpected token: '{}'.", source.substr(start, current - start)));
            hasError = true;
            return std::nullopt;

        // Negative numbers.
        case '-':
            if (auto num = lexNumber())
                return num;
            diag.error(line, fmt::format("Expected number after '-'."));
            hasError = true;
            return std::nullopt;

        default:
            if (isdigit(c))
                return lexNumber();

            if (auto kw = lexKeyword())
                return kw;

            diag.error(line, fmt::format("Unexpected token: '{}'.", source.substr(start, current - start)));
            hasError = true;
            return std::nullopt;
        }

        if (isAtEnd())
            return std::nullopt;
    }
}

std::optional<Token> Lexer::lexNumber() noexcept
{
    while (isdigit(peek()))
        advance();

    int value = atoi(source.substr(start, current - start).c_str());

    return Token(NUMBER, line, value);
}

namespace {
const std::unordered_map<std::string_view, TokenType> keywords = {
    {tokenTypeToSourceName(INIT),    INIT},
    {tokenTypeToSourceName(OR),     OR},
    {tokenTypeToSourceName(TRANSLATION),  TRANSLATION},
    {tokenTypeToSourceName(ROTATION),  ROTATION},
    {tokenTypeToSourceName(ITER),    ITER},
};
} // anonymous namespace

std::optional<Token> Lexer::lexKeyword() noexcept
{
    while (isalpha(peek()))
        advance();

    auto text = source.substr(start, current - start);
    if (auto it = keywords.find(text); it != keywords.end())
        return Token(it->second, line);

    return {};
}

bool Lexer::match(char expected) noexcept
{
    if (isAtEnd())
        return false;
    if (source[current] != expected)
        return false;

    current++;
    return true;
}

char Lexer::peek() const noexcept
{
    if (isAtEnd())
        return '\0';
    return source[current];
}

char Lexer::peekNext() const noexcept
{
    if (static_cast<unsigned>(current + 1) >= source.length())
        return '\0';
    return source[current + 1];
}
