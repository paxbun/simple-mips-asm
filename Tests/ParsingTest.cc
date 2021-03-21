// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <simple-mips-asm/Parsing.hh>
#include <simple-mips-asm/Tokenization.hh>

#include "TestCommon.hh"
#include <cstring>

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
sum_exit:
    addu $4, $3, $0
    jr $31
exit:
)==";

// ------------------------------  Fragment comparison operators ------------------------------- //

#pragma region Comparison Opreators

#define DEFINE_BITWISE_EQUALITY(Type)                                                              \
    bool operator==(Type const& lhs, Type const& rhs) noexcept                                     \
    {                                                                                              \
        return memcmp(std::addressof(lhs), std::addressof(rhs), sizeof(Type)) == 0;                \
    }

constexpr bool operator==(DataDirData const&, DataDirData const&) noexcept
{
    return true;
}

constexpr bool operator==(TextDirData const&, TextDirData const&) noexcept
{
    return true;
}

DEFINE_BITWISE_EQUALITY(WordDirData);

constexpr bool operator==(LabelData const& lhs, LabelData const& rhs) noexcept
{
    return lhs.value == rhs.value;
}

DEFINE_BITWISE_EQUALITY(RFormatData);
DEFINE_BITWISE_EQUALITY(JRFormatData);
DEFINE_BITWISE_EQUALITY(SRFormatData);
DEFINE_BITWISE_EQUALITY(IFormatData);

constexpr bool operator==(BIFormatData const& lhs, BIFormatData const& rhs) noexcept
{
    return lhs.operation == rhs.operation && lhs.destination == rhs.destination
           && lhs.source == rhs.source && lhs.target == rhs.target;
}

DEFINE_BITWISE_EQUALITY(IIFormatData);
DEFINE_BITWISE_EQUALITY(OIFormatData);

constexpr bool operator==(JFormatData const& lhs, JFormatData const& rhs) noexcept
{
    return lhs.operation == rhs.operation && lhs.target == rhs.target;
}

constexpr bool operator==(LAFormatData const& lhs, LAFormatData const& rhs) noexcept
{
    return lhs.type == rhs.type && lhs.destination == rhs.destination && lhs.target == rhs.target;
}

#pragma endregion Comparison Operators

// ------------------------------------------  Tests ------------------------------------------- //

TEST(ParsingTest, ValidCode1)
{
    auto tokenizationResult = Tokenize(_validCode1);
    ASSERT_TRUE(tokenizationResult.errors.empty());

    auto parsingResult = Parse(tokenizationResult.tokens);
    ASSERT_TRUE(parsingResult.errors.empty());

    // clang-format off
    std::vector<FragmentData> expected {
        // .data
        DataDirData {},
        // array: .word 3
        LabelData { "array" }, WordDirData { 3 },
        // .word 123
        WordDirData { 123 },
        // .word 4346
        WordDirData { 4346 },
        // array2: .word 0x12345678
        LabelData { "array2" }, WordDirData { 0x12345678 },
        // .word 0xFFFFFFFF
        WordDirData { 0xFFFFFFFF },
        // .text
        TextDirData {},
        // main:
        LabelData { "main" },
        // addiu $2, $0, 1024
        IFormatData { IFormatOperation::ADDIU, 2, 0, 1024 },
        // addu $3, $2, $2
        RFormatData { RFormatFunction::ADDU, 3, 2, 2 },
        // or $4, $3, $2
        RFormatData { RFormatFunction::OR, 4, 3, 2 },
        // addiu $5, $0, 1234
        IFormatData { IFormatOperation::ADDIU, 5, 0, 1234 },
        // sll $6, $5, 16
        SRFormatData { SRFormatFunction::SLL, 6, 5, 16 },
        // addiu $7, $6, 9999
        IFormatData { IFormatOperation::ADDIU, 7, 6, 9999 },
        // subu $8, $7, $2
        RFormatData { RFormatFunction::SUBU, 8, 7, 2 },
        // nor $9, $4, $3
        RFormatData { RFormatFunction::NOR, 9, 4, 3 },
        // ori $10, $2, 255
        IFormatData { IFormatOperation::ORI, 10, 2, 255 },
        // srl $11, $6, 5
        SRFormatData { SRFormatFunction::SRL, 11, 6, 5 },
        // srl $12, $6, 4
        SRFormatData { SRFormatFunction::SRL, 12, 6, 4 },
        // la $4, array2
        LAFormatData { LAFormatType::LA, 4, "array2" },
        // lb $2, 1($4)
        OIFormatData { OIFormatOperation::LB, 2, 1, 4 },
        // sb $2, 6($4)
        OIFormatData { OIFormatOperation::SB, 2, 6, 4 },
        // and $13, $11, $5
        RFormatData { RFormatFunction::AND, 13, 11, 5 },
        // andi $14, $4, 100
        IFormatData { IFormatOperation::ANDI, 14, 4, 100 },
        // subu $15, $0, $10
        RFormatData { RFormatFunction::SUBU, 15, 0, 10 },
        // lui $17, 100
        IIFormatData { IIFormatOperation::LUI, 17, 100 },
        // addiu $2, $0, 0xa
        IFormatData { IFormatOperation::ADDIU, 2, 0, 0xA },
    };
    // clang-format on

    auto const& fragments = parsingResult.fragments;
    ASSERT_EQ_VECTOR(fragments, expected, lit->data, *rit);
}

TEST(ParsingTest, ValidCode2)
{
    auto tokenizationResult = Tokenize(_validCode2);
    ASSERT_TRUE(tokenizationResult.errors.empty());

    auto parsingResult = Parse(tokenizationResult.tokens);
    ASSERT_TRUE(parsingResult.errors.empty());

    // clang-format off
    std::vector<FragmentData> expected {
        // .data
        DataDirData {},
        // var: .word 5
        LabelData { "var" }, WordDirData { 5 },
        // .text
        TextDirData {},
        // main:
        LabelData { "main" },
        // la $8, var
        LAFormatData { LAFormatType::LA, 8, "var" },
        // lw $9, 0($8)
        OIFormatData { OIFormatOperation::LW, 9, 0, 8 },
        // addu $2, $0, $9
        RFormatData { RFormatFunction::ADDU, 2, 0, 9 },
        // jal sum
        JFormatData { JFormatOperation::JAL, "sum" },
        // j exit
        JFormatData { JFormatOperation::J, "exit" },
        // sum: sltiu $1, $2, 1
        LabelData { "sum" }, IFormatData { IFormatOperation::SLTIU, 1, 2, 1 },
        // bne: $1, $0, sum_exit
        BIFormatData { BIFormatOperation::BNE, 1, 0, "sum_exit" },
        // addu $3, $3, $2
        RFormatData { RFormatFunction::ADDU, 3, 3, 2 },
        // addiu $2, $2, -1
        IFormatData { IFormatOperation::ADDIU, 2, 2, 65535/* -1 */ },
        // j sum
        JFormatData { JFormatOperation::J, "sum" },
        // sum_exit:
        LabelData { "sum_exit" },
        // addu $4, $3, $0
        RFormatData { RFormatFunction::ADDU, 4, 3, 0 },
        // jr $31
        JRFormatData { JRFormatFunction::JR, 31 },
        // exit:
        LabelData { "exit" },
    };
    // clang-format on

    auto const& fragments = parsingResult.fragments;
    ASSERT_EQ_VECTOR(fragments, expected, lit->data, *rit);
}
