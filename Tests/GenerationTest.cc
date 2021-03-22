// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <simple-mips-asm/Generation.hh>
#include <simple-mips-asm/Parsing.hh>
#include <simple-mips-asm/Tokenization.hh>

#include "TestCommon.hh"

// ------------------------------------------  Codes ------------------------------------------- //

char const _validCode1[] = R"==(
        .data
array:  .word   3
        .word   123
        .word   4346
array2: .word   0x12345678
        .word   0xFFFFFFFF
        .text
main:
        addiu   $2, $0, 1024
        addu    $3, $2, $2
        or      $4, $3, $2
        addiu   $5, $0, 1234
        sll     $6, $5, 16
        addiu   $7, $6, 9999
        subu    $8, $7, $2
        nor     $9, $4, $3
        ori     $10, $2, 255
        srl     $11, $6, 5
        srl     $12, $6, 4
        la      $4, array2
        lb      $2, 1($4)
        sb      $2, 6($4)
        and     $13, $11, $5
        andi    $14, $4, 100
        subu    $15, $0, $10
        lui     $17, 100
        addiu   $2, $0, 0xa
)==";

char const _validCode2[] = R"==(
        .data
var:  .word   5
        .text
main:
    la $8, var
    lw $9, 0($8)
    addu $2, $0, $9
    jal sum
    j exit

sum: sltiu $1, $2, 1
    bne $1, $0, sum_exit
    addu $3, $3, $2
    addiu $2, $2, -1
    j sum
    beq $3, $15, sum
sum_exit:
    addu $4, $3, $0
    jr $31
exit:)==";

TEST(GenerationTest, ValidCode1)
{
    auto tokenizationResult = Tokenize(_validCode1);
    ASSERT_TRUE(tokenizationResult.errors.empty());

    auto parsingResult = Parse(tokenizationResult.tokens);
    ASSERT_TRUE(parsingResult.errors.empty());

    auto generationResult = GenerateCode(parsingResult.fragments);
    ASSERT_TRUE(std::holds_alternative<CanGenerate>(generationResult));
    auto const& code = std::get<CanGenerate>(generationResult);

    {
        std::vector<uint32_t> expected { 3, 123, 4346, 0x12345678, 0xFFFFFFFF };
        auto const&           data = code.data;
        ASSERT_EQ_VECTOR(data, expected, *lit, *rit);
    }

    {
        // clang-format off
        std::vector<uint32_t> expected {
            // main:
            // addiu $2, $0, 1024
            0b001001'00000'00010'0000010000000000u,
            // addu $3, $2, $2
            0b000000'00010'00010'00011'00000'100001u,
            // or $4, $3, $2
            0b000000'00011'00010'00100'00000'100101u,
            // addiu $5, $0, 1234
            0b001001'00000'00101'0000010011010010u,
            // sll $6, $5, 16
            0b000000'00000'00101'00110'10000'000000u,
            // addiu $7, $6, 9999
            0b001001'00110'00111'0010011100001111u,
            // subu $8, $7, $2
            0b000000'00111'00010'01000'00000'100011u,
            // nor $9, $4, $3
            0b000000'00100'00011'01001'00000'100111u,
            // ori $10, $2, 255
            0b001101'00010'01010'0000000011111111u,
            // srl $11, $6, 5
            0b000000'00000'00110'01011'00101'000010u,
            // srl $12, $6, 4
            0b000000'00000'00110'01100'00100'000010u,
            // la $4, array2
            //     lui $4, 0x1000
            0b001111'00000'00100'0001000000000000u,
            //     ori $4, $4, 0x000C
            0b001101'00100'00100'0000000000001100u,
            // lb $2, 1($4)
            0b100000'00100'00010'0000000000000001u,
            // sb $2, 6($4)
            0b101000'00100'00010'0000000000000110u,
            // and $13, $11, $5
            0b000000'01011'00101'01101'00000'100100u,
            // andi $14, $4, 100
            0b001100'00100'01110'0000000001100100u,
            // subu $15, $0, $10
            0b000000'00000'01010'01111'00000'100011u,
            // lui $17, 100
            0b001111'00000'10001'0000000001100100u,
            // addiu $2, $0, 0xa
            0b001001'00000'00010'0000000000001010u,
        };
        // clang-format on
        auto const& text = code.text;
        ASSERT_EQ_VECTOR(text, expected, *lit, *rit);
    }
}

TEST(GenerationTest, ValidCode2)
{
    auto tokenizationResult = Tokenize(_validCode2);
    ASSERT_TRUE(tokenizationResult.errors.empty());

    auto parsingResult = Parse(tokenizationResult.tokens);
    ASSERT_TRUE(parsingResult.errors.empty());

    auto generationResult = GenerateCode(parsingResult.fragments);
    ASSERT_TRUE(std::holds_alternative<CanGenerate>(generationResult));
    auto const& code = std::get<CanGenerate>(generationResult);

    {
        std::vector<uint32_t> expected { 5 };
        auto const&           data = code.data;
        ASSERT_EQ_VECTOR(data, expected, *lit, *rit);
    }

    {
        // clang-format off
        std::vector<uint32_t> expected {
            // main:
            // la $8, var
            //     lui $8, 0x1000
            0b001111'00000'01000'0001000000000000u,
            // lw $9, 0($8)
            0b100011'01000'01001'0000000000000000u,
            // addu $2, $0, $9
            0b000000'00000'01001'00010'00000'100001u,
            // jal sum
            0b000011'00000100000000000000000101u,
            // j exit
            0b000010'00000100000000000000001101u,
            // sum: sltiu $1, $2, 1
            0b001011'00010'00001'0000000000000001u,
            // bne $1, $0, sum_exit
            0b000101'00001'00000'0000000000000100u,
            // addu $3, $3, $2
            0b000000'00011'00010'00011'00000'100001u,
            // addiu $2, $2, -1
            0b001001'00010'00010'1111111111111111u,
            // j sum
            0b000010'00000100000000000000000101u,
            // beq $3, $15, sum
            0b000100'00011'01111'1111111111111010u,
            // sum_exit:
            // addu $4, $3, $0
            0b000000'00011'00000'00100'00000'100001u,
            // jr $31
            0b000000'11111'000000000000000'001000u,
        };
        // clang-format on
        auto const& text = code.text;
        ASSERT_EQ_VECTOR(text, expected, *lit, *rit);
    }
}