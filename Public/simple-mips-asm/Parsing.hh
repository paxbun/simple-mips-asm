// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef SIMPLE_MIPS_ASM_PARSE_HH
#define SIMPLE_MIPS_ASM_PARSE_HH

#include <simple-mips-asm/Formats.hh>
#include <simple-mips-asm/Tokenization.hh>

#include <string_view>
#include <variant>
#include <vector>

// ----------------------------------- Fragment definitions ------------------------------------ //

struct DataDirData
{};

struct TextDirData
{};

struct WordDirData
{
    uint32_t value;
};

struct LabelData
{
    std::string_view value;
};

struct RFormatData
{
    uint8_t         source1;
    uint8_t         source2;
    uint8_t         destination;
    RFormatFunction function;
};

struct JRFormatData
{
    uint8_t          source;
    JRFormatFunction function;
};

struct SRFormatData
{
    uint8_t          source;
    uint8_t          destination;
    uint8_t          shiftAmount;
    SRFormatFunction function;
};

struct IFormatData
{
    IFormatOperation operation;
    uint8_t          destination;
    uint8_t          source;
    uint16_t         immediate;
};

struct BIFormatData
{
    BIFormatOperation operation;
    uint8_t           destination;
    uint8_t           source;
    std::string_view  target;
};

struct IIFormatData
{
    IIFormatOperation operation;
    uint8_t           destination;
    uint16_t          immediate;
};

struct OIFormatData
{
    OIFormatOperation operation;
    uint8_t           operand1;
    uint8_t           operand2;
    uint16_t          offset;
};

struct JFormatData
{
    JFormatOperation operation;
    std::string_view target;
};

struct LAFormatData
{
    LAFormatType     type;
    uint8_t          destination;
    std::string_view target;
};

// clang-format off
using FragmentData = std::variant<
    // directives
    DataDirData,  TextDirData,  WordDirData,
    // labels
    LabelData,
    // instructions
    RFormatData,  JRFormatData, SRFormatData,
    IFormatData,  BIFormatData, IIFormatData,
    OIFormatData, JFormatData,  LAFormatData>;
// clang-format on

/// <summary>
/// Represents a part of code which is an instruction, a label, or a directive.
/// </summary>
struct Fragment
{
    FragmentData data;
    Range        range;
};

/// <summary>
/// Represents an error occurred during the parsing phase.
/// </summary>
struct ParsingError
{
    enum class Type
    {
        UnexpectedToken,
        UnexpectedEof,
        UnexpectedValue,
    };

    Type  type;
    Range range;
};

/// <summary>
/// Represents a parsing result.
/// </summary>
struct ParseResult
{
    std::vector<Fragment>     fragments;
    std::vector<ParsingError> errors;
};

/// <summary>
/// Parses the given array of tokens. The given array's lifetime must be equal to or longer than
/// that of fragments.
/// </summary>
/// <param name="tokens">the array of tokens to parse</param>
/// <returns>parsing result</returns>
ParseResult Parse(std::vector<Token> const& tokens);

#endif