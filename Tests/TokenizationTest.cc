// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <simple-mips-asm/Tokenization.hh>

#include "TestCommon.hh"

#define T(n) Token::Type::n

char const _validCode[] = R"==(
       .data
array: .word 1
       .word 0x12
       .text
main:  addiu $2, $3, 14
       addu  $3, $1, $10
       lb    $2, 0x5($4)
)==";

char const _codeWithInvalidFormat[] = R"==(
    .data
    .word 0xaQWe
    .text addiu $3, $1, 10a
)==";

constexpr bool operator==(Position const& lhs, Position const& rhs) noexcept
{
    return lhs.line == rhs.line && lhs.character == rhs.character;
}

constexpr bool operator==(Range const& lhs, Range const& rhs) noexcept
{
    return lhs.begin == rhs.begin && lhs.end == rhs.end;
}

TEST(TokenizationTest, ValidCode)
{
    auto result = Tokenize(_validCode);
    ASSERT_TRUE(result.errors.empty());

    // clang-format off
    std::vector<Token::Type> expected {
        T(NewLine),
        // .data
        T(Whitespace), T(Dot), T(Word), T(NewLine),
        // array: .word 1
        T(Word), T(Colon), T(Whitespace), T(Dot), T(Word), T(Whitespace), T(Integer), T(NewLine),
        // .word 0x12
        T(Whitespace), T(Dot), T(Word), T(Whitespace), T(HexInteger), T(NewLine),
        // .text
        T(Whitespace), T(Dot), T(Word), T(NewLine),
        // main: addiu $2, $3, 14
        T(Word), T(Colon), T(Whitespace), T(Word), T(Whitespace),
            T(Dollar), T(Integer), T(Comma), T(Whitespace),
            T(Dollar), T(Integer), T(Comma), T(Whitespace),
            T(Integer), T(NewLine),
        // addu $3, $1, $10
        T(Whitespace), T(Word), T(Whitespace),
            T(Dollar), T(Integer), T(Comma), T(Whitespace),
            T(Dollar), T(Integer), T(Comma), T(Whitespace),
            T(Dollar), T(Integer), T(NewLine),
        // lb $2 0x5($4)
        T(Whitespace), T(Word), T(Whitespace),
            T(Dollar), T(Integer), T(Comma), T(Whitespace),
            T(HexInteger), T(BracketOpen), T(Dollar), T(Integer), T(BracketClose), T(NewLine),
    };
    // clang-format on

    auto const& tokens = result.tokens;
    ASSERT_EQ_VECTOR(tokens, expected, lit->type, *rit);
}

TEST(TokenizationTest, CodeWithInvalidFormat)
{
    auto result = Tokenize(_codeWithInvalidFormat);

    {
        std::vector<Range> expected {
            Range { 3, 11, 3, 17 },
            Range { 4, 25, 4, 28 },
        };

        auto const& errors = result.errors;
        ASSERT_EQ_VECTOR(errors, expected, lit->range, *rit);
    }

    {
        // clang-format off
        std::vector<Token::Type> expected {
            T(NewLine),
            // .data
            T(Whitespace), T(Dot), T(Word), T(NewLine),
            // .word 0xaQWe
            T(Whitespace), T(Dot), T(Word), T(Whitespace), T(HexInteger), T(NewLine),
            // .text addiu $3, $1, 10a
            T(Whitespace), T(Dot), T(Word), T(Whitespace),
                T(Word), T(Whitespace),
                T(Dollar), T(Integer), T(Comma), T(Whitespace),
                T(Dollar), T(Integer), T(Comma), T(Whitespace),
                T(Integer), T(NewLine),
        };
        // clang-format on

        auto const& tokens = result.tokens;
        ASSERT_EQ_VECTOR(tokens, expected, lit->type, *rit);
    }
}