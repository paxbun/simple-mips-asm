// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef SIMPLE_MIPS_ASM_TOKEN_HH
#define SIMPLE_MIPS_ASM_TOKEN_HH

#include <stdexcept>
#include <string_view>
#include <vector>

/// <summary>
/// Represents a token
/// </summary>
struct Token
{
    /// <summary>
    /// Represents the type of the given token
    /// </summary>
    enum class Type
    {
        Dot,          // a single dot
        Colon,        // a single colon
        Dollar,       // a single dollar sign
        BracketOpen,  // a left round bracket
        BracketClose, // a right round bracket
        Integer,      // (\d)+
        Word,         // [a-zA-Z]([0-9a-zA-Z])*
        Whitespace,
    };

    Type             type;
    std::string_view value;
};

/// <summary>
/// Thrown when the tokenizer encountered an unexpected character.
/// </summary>
struct TokenizationException : std::exception
{
    TokenizationException() = default;

    virtual const char* what()
    {
        return "Encountered an unexpected character";
    }
};

/// <summary>
/// Tokenizes the given assembly code. The given string's lifetime must be equal to or longer than
/// that of tokens.
/// </summary>
/// <param name="code">the assembly code to tokenize</param>
/// <returns>tokenization result</returns>
std::vector<Token> Tokenize(std::string const& code);

#endif