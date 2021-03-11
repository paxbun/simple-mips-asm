// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef SIMPLE_MIPS_ASM_CONSTANTS_HH
#define SIMPLE_MIPS_ASM_CONSTANTS_HH

#include <cstdint>

/// <summary>
/// Represents the type of the given R format instruction
/// </summary>
enum class RFormatFunction : uint8_t
{
    ADDU = 0x21,
    SUBU = 0x23,
    AND  = 0x24,
    OR   = 0x25,
    NOR  = 0x27,
    SLTU = 0x2B,
};

/// <summary>
/// Represents the type of the given JR format instruction
/// </summary>
enum class JRFormatFunction : uint8_t
{
    JR = 0x08,
};

/// <summary>
/// Represents the type of the given SR format instruction
/// </summary>
enum class SRFormatFunction : uint8_t
{
    SLL = 0x00,
    SRL = 0x02,
};

/// <summary>
/// Represents the type of the given I format instruction
/// </summary>
enum class IFormatOperation : uint8_t
{
    ADDIU = 0x09,
    ANDI  = 0x0C,
    ORI   = 0x0D,
    SLTIU = 0x0B,
};

/// <summary>
/// Represents the type of the given BI format instruction
/// </summary>
enum class BIFormatOperation : uint8_t
{
    BEQ = 0x04,
    BNE = 0x05,
};

/// <summary>
/// Represents the type of the given II format instruction
/// </summary>
enum class IIFormatOperation : uint8_t
{
    LUI = 0x0F,
};

/// <summary>
/// Represents the type of the given OI format instruction
/// </summary>
enum class OIFormatOperation : uint8_t
{
    LB = 0x20,
    LW = 0x23,
    SB = 0x28,
    SW = 0x2B,
};

/// <summary>
/// Represents the type of the given J format instruction
/// </summary>
enum class JFormatOperation : uint8_t
{
    J   = 0x02,
    JAL = 0x03,
};

/// <summary>
/// Represents the type of the given LA format instruction
/// </summary>
enum class LAFormatType : uint8_t
{
    LA
};

#endif