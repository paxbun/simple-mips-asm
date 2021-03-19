// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef SIMPLE_MIPS_ASM_GENERATOR_HH
#define SIMPLE_MIPS_ASM_GENERATOR_HH

#include <simple-mips-asm/Parsing.hh>

#include <cstdint>
#include <cstring>
#include <variant>
#include <vector>

struct GenerationError
{
    enum class Type
    {
        UndefinedLabelName,
        OperandIsLabelInTextSegment,
        LabelAlreadyDefined,
        BranchTargetTooFar,
        JumpAddressTooBig,
    };

    Type  type;
    Range range;
};

struct CanGenerate
{
    std::vector<uint32_t> data;
    std::vector<uint32_t> text;
};

struct CannotGenerate
{
    std::vector<GenerationError> errors;
};

using GenerationResult = std::variant<CanGenerate, CannotGenerate>;

/// <summary>
/// Generates machine code from the given array of fragments.
/// </summary>
/// <param name="fragments">the array of fragments</param>
/// <returns>generation result</returns>
GenerationResult GenerateCode(std::vector<Fragment> const& fragments);

#endif