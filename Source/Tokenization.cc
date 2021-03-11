// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <simple-mips-asm/Common.hh>
#include <simple-mips-asm/Tokenization.hh>

#include <algorithm>
#include <cctype>
#include <variant>

namespace
{

// ---------------------------------- Tokenizer output types ----------------------------------- //

/// <summary>
/// Returned when the tokenizer cannot recognize the given string.
/// </summary>
struct TokenizerCannotRecognize
{};

/// <summary>
/// Returned when the tokenizer can recognize the given string, and the token has a valid format.
/// </summary>
struct TokenizerCanRecognize
{
    StringIterator tokenEnd;
    Token::Type    tokenType;
};

/// <summary>
/// Returned when the tokenizer can recognize the given string, but the token has an invalid format.
/// </summary>
struct TokenizerCanRecognizeButError : TokenizerCanRecognize
{
    TokenizationError::Type errorType;
};

/// <summary>
/// Union of possible tokenizer output types
/// </summary>
using TokenizerOutput
    = std::variant<TokenizerCannotRecognize, TokenizerCanRecognize, TokenizerCanRecognizeButError>;

// ---------------------------------------- Tokenizers ----------------------------------------- //

using StringIterator = std::string::const_iterator;

/// <summary>
/// Token pattern matcher
/// </summary>
using Tokenizer = TokenizerOutput (*)(StringIterator, StringIterator);

template <Token::Type TokenTypeValue, char Character>
TokenizerOutput SingleCharacterTokenizer(StringIterator begin, StringIterator end)
{
    if (*begin != Character) return TokenizerCannotRecognize {};
    return TokenizerCanRecognize { begin + 1, TokenTypeValue };
}

TokenizerOutput HexIntegerTokenizer(StringIterator begin, StringIterator end)
{
    if (std::distance(begin, end) >= 2 && begin[0] == '0' && begin[1] == 'x')
    {
        auto tokenEnd = std::find_if(begin, end, [](char c) { return isspace(c); });

        auto nonHexadecimalCharIt = std::find_if(begin + 2, tokenEnd, [](char c) {
            return isdigit(c) || ('A' <= toupper(c) && toupper(c) <= 'F');
        });
        if (nonHexadecimalCharIt != tokenEnd)
        {
            return TokenizerCanRecognizeButError {
                tokenEnd,
                Token::Type::HexInteger,
                TokenizationError::Type::InvalidFormat,
            };
        }

        return TokenizerCanRecognize { tokenEnd, Token::Type::HexInteger };
    }

    return TokenizerCannotRecognize {};
}

#define DEFINE_COMPLEX_TOKENIZER(InitialCondition, Verification, TokenType)                        \
    TokenizerOutput TokenType##Tokenizer(StringIterator begin, StringIterator end)                 \
    {                                                                                              \
        if (InitialCondition)                                                                      \
        {                                                                                          \
            auto tokenEnd = std::find_if(begin, end, [](char c) { return isspace(c); });           \
            auto invalidCharIt                                                                     \
                = std::find_if_not(begin, end, [](char c) { return (Verification); });             \
            if (invalidCharIt != tokenEnd)                                                         \
            {                                                                                      \
                return TokenizerCanRecognizeButError {                                             \
                    tokenEnd,                                                                      \
                    Token::Type::TokenType,                                                        \
                    TokenizationError::Type::InvalidFormat,                                        \
                };                                                                                 \
            }                                                                                      \
            return TokenizerCanRecognize { tokenEnd, Token::Type::TokenType };                     \
        }                                                                                          \
        return TokenizerCannotRecognize {};                                                        \
    }

DEFINE_COMPLEX_TOKENIZER(isdigit(*begin), isdigit(c), Integer);

DEFINE_COMPLEX_TOKENIZER(isalpha(*begin), (isalpha(c) || isdigit(c)), Word);

TokenizerOutput WhitespaceTokenizer(StringIterator begin, StringIterator end)
{
    if (isspace(*begin) && *begin != '\n')
    {
        auto tokenEnd
            = std::find_if_not(begin, end, [](char c) { return isspace(c) && c != '\n'; });

        return TokenizerCanRecognize { tokenEnd, Token::Type::Whitespace };
    }
    return TokenizerCannotRecognize {};
}

Tokenizer _tokenizers[] = {
    SingleCharacterTokenizer<Token::Type::Dot, '.'>,
    SingleCharacterTokenizer<Token::Type::Colon, ':'>,
    SingleCharacterTokenizer<Token::Type::Dollar, '$'>,
    SingleCharacterTokenizer<Token::Type::BracketOpen, '('>,
    SingleCharacterTokenizer<Token::Type::BracketClose, ')'>,
    SingleCharacterTokenizer<Token::Type::NewLine, '\n'>,
    HexIntegerTokenizer,
    IntegerTokenizer,
    WordTokenizer,
    WhitespaceTokenizer,
};

// ----------------------------------------  Utilities ----------------------------------------- //

Token MakeToken(Token::Type type, StringIterator begin, StringIterator end, Position& position)
{
    Position beginPos = position, endPos = position;
    for (auto it = begin; it != end; ++it)
    {
        if (*it == '\n') endPos = endPos.NextLine();
        else
            endPos = endPos.MoveRight();
    }
    position = endPos;

    return Token {
        type,
        { beginPos, endPos },
        std::string_view(std::addressof(*begin), std::distance(begin, end)),
    };
}

}

TokenizationResult Tokenize(std::string const& code)
{
    auto       begin = code.begin();
    auto const end   = code.end();

    Position position;

    std::vector<Token>             tokens;
    std::vector<TokenizationError> errors;

    while (begin != end)
    {
        bool tokenized = false;
        for (auto tokenizer : _tokenizers)
        {
            auto result = tokenizer(begin, end);

            if (std::holds_alternative<TokenizerCanRecognize>(result))
            {
                auto output = std::get<TokenizerCanRecognize>(result);
                tokenized   = true;
                tokens.push_back(MakeToken(output.tokenType, begin, output.tokenEnd, position));
                begin = output.tokenEnd;
                break;
            }
            else if (std::holds_alternative<TokenizerCanRecognizeButError>(result))
            {
                auto output = std::get<TokenizerCanRecognizeButError>(result);
                tokenized   = true;
                auto token  = MakeToken(output.tokenType, begin, output.tokenEnd, position);
                tokens.push_back(token);
                errors.push_back({
                    output.errorType,
                    token.range,
                });
                begin = output.tokenEnd;
                break;
            }
        }

        if (!tokenized)
        {
            auto newPosition = position.MoveRight();
            if (*begin == '\n') newPosition = position.NextLine();

            errors.push_back({
                TokenizationError::Type::InvalidCharacter,
                { position, newPosition },
            });

            position = newPosition;
            ++begin;
        }
    }

    return { tokens, errors };
}
