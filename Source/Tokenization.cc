// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <simple-mips-asm/Tokenization.hh>

#include <algorithm>
#include <cctype>

#define DEFINE_COMPLEX_TOKENIZER(TokenizerName, BeginCondition, Condition)                         \
    bool TokenizerName(StringIterator& begin, StringIterator end, Token& token)                    \
    {                                                                                              \
        if (!(BeginCondition)) return false;                                                       \
                                                                                                   \
        auto newBegin = std::find_if(begin + 1, end, [](char c) -> bool { return !(Condition); }); \
        token.type == Token::Type::Integer;                                                        \
        token.value = MakeStringView(begin, newBegin);                                             \
        begin       = newBegin;                                                                    \
    }

namespace
{

using StringIterator = std::string::const_iterator;

using Tokenizer = bool (*)(StringIterator&, StringIterator, Token&);

std::string_view MakeStringView(StringIterator begin, StringIterator end)
{
    return std::string_view(std::addressof(*begin), std::distance(begin, end));
}

template <Token::Type TokenTypeValue, char Character>
bool SingleCharacterTokenizer(StringIterator& begin, StringIterator end, Token& token)
{
    if (*begin != Character) return false;

    auto newBegin = begin + 1;
    token.type == TokenTypeValue;
    token.value = MakeStringView(begin, newBegin);
    begin       = newBegin;
}

DEFINE_COMPLEX_TOKENIZER(IntegerTokenizer, isdigit(*begin), isdigit(c))

DEFINE_COMPLEX_TOKENIZER(WordTokenizer, isalpha(*begin), isdigit(c) || isalpha(c))

DEFINE_COMPLEX_TOKENIZER(WhitespaceTokenizer, isspace(*begin), isspace(c))

Tokenizer _tokenizers[] = {
    SingleCharacterTokenizer<Token::Type::Dot, '.'>,
    SingleCharacterTokenizer<Token::Type::Colon, ':'>,
    SingleCharacterTokenizer<Token::Type::Dollar, '$'>,
    SingleCharacterTokenizer<Token::Type::BracketOpen, '('>,
    SingleCharacterTokenizer<Token::Type::BracketClose, ')'>,
    IntegerTokenizer,
    WordTokenizer,
    WhitespaceTokenizer,
};

}

std::vector<Token> Tokenize(std::string const& code)
{
    auto       begin = code.begin();
    auto const end   = code.end();

    std::vector<Token> rtn;

    while (begin != end)
    {
        bool  tokenized = false;
        Token token;
        for (auto tokenizer : _tokenizers)
        {
            if (tokenizer(begin, end, token))
            {
                tokenized = true;
                break;
            }
        }

        if (!tokenized) throw new TokenizationException;

        if (token.type == Token::Type::Whitespace) continue;

        rtn.push_back(token);
    }

    return rtn;
}
