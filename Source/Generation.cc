// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <simple-mips-asm/Generation.hh>

#include <algorithm>
#include <iterator>
#include <string_view>
#include <unordered_map>

namespace
{

// ---------------------------------  Instruction definitions ---------------------------------- //

struct RFormatInstr
{
    uint32_t : 6;
    uint32_t source1 : 5;
    uint32_t source2 : 5;
    uint32_t destination : 5;
    uint32_t : 5;
    uint32_t function : 6;
};

struct JRFormatInstr
{
    uint32_t : 6;
    uint32_t source : 5;
    uint32_t : 15;
    uint32_t function : 6;
};

struct SRFormatInstr
{
    uint32_t : 11;
    uint32_t source : 5;
    uint32_t destination : 5;
    uint32_t shiftAmount : 5;
    uint32_t function : 6;
};

struct IFormatInstr
{
    uint32_t operation : 6;
    uint32_t source : 5;
    uint32_t destination : 5;
    uint32_t immediate : 16;
};

struct BIFormatInstr
{
    uint32_t operation : 6;
    uint32_t source : 5;
    uint32_t destination : 5;
    uint32_t offset : 16;
};

struct IIFormatInstr
{
    uint32_t operation : 6;
    uint32_t : 5;
    uint32_t destination : 5;
    uint32_t immediate : 16;
};

struct OIFormatInstr
{
    uint32_t operation : 6;
    uint32_t operand1 : 5;
    uint32_t operand2 : 5;
    uint32_t offset : 16;
};

struct JFormatInstr
{
    uint32_t operation : 6;
    uint32_t target : 26;
};

static_assert(sizeof(RFormatInstr) == sizeof(uint32_t));

static_assert(sizeof(JRFormatInstr) == sizeof(uint32_t));

static_assert(sizeof(SRFormatInstr) == sizeof(uint32_t));

static_assert(sizeof(IFormatInstr) == sizeof(uint32_t));

static_assert(sizeof(BIFormatInstr) == sizeof(uint32_t));

static_assert(sizeof(IIFormatInstr) == sizeof(uint32_t));

static_assert(sizeof(OIFormatInstr) == sizeof(uint32_t));

static_assert(sizeof(JFormatInstr) == sizeof(uint32_t));

template <typename Instr>
union InstrConverter
{
    Instr    instr;
    uint32_t word;

    static_assert(sizeof(Instr) == sizeof(uint32_t));

    constexpr InstrConverter() noexcept : word { 0 } {}

    constexpr Instr* operator->() noexcept
    {
        return &instr;
    }

    constexpr operator uint32_t() const noexcept
    {
        return word;
    }
};

// ----------------------------------------  Utilities ----------------------------------------- //

/// <summary>
/// Represents an address.
/// </summary>
struct Address
{
    enum class BaseType : uint32_t
    {
        TextSegment = 0x400000,
        DataSegment = 0x10000000
    };

    BaseType base;
    uint32_t offset;

    constexpr void MoveToNext() noexcept
    {
        offset += 4;
    }

    constexpr operator uint32_t() const noexcept
    {
        return static_cast<uint32_t>(base) + offset;
    }
};

/// <summary>
/// A key-value storage where the key is a label and the value is an address.
/// </summary>
using LabelTable = std::unordered_map<std::string_view, Address>;

/// <summary>
/// Scanning result
/// </summary>
struct ScanResult
{
    uint32_t                     numDataWords;
    uint32_t                     numTextWords;
    LabelTable                   labelTable;
    std::vector<GenerationError> errors;
};

/// <summary>
/// Scans the given array of fragments, calculates the positions of the labels, number of words
/// needed to store data segment and text segment.
/// </summary>
/// <param name="fragments">the array of fragments to scan</param>
/// <returns>scanning result</returns>
ScanResult ScanFragments(std::vector<Fragment> const& fragments)
{
    uint32_t                     numDataWords = 0;
    uint32_t                     numTextWords = 0;
    LabelTable                   labelTable;
    std::vector<GenerationError> errors;

    Address::BaseType base                 = Address::BaseType::TextSegment;
    uint32_t*         pNumWordsToIncrement = &numTextWords;

    for (auto& fragment : fragments)
    {
        if (std::holds_alternative<DataDirData>(fragment.data))
        {
            base                 = Address::BaseType::DataSegment;
            pNumWordsToIncrement = &numDataWords;
        }
        else if (std::holds_alternative<TextDirData>(fragment.data))
        {
            base                 = Address::BaseType::TextSegment;
            pNumWordsToIncrement = &numTextWords;
        }
        else if (std::holds_alternative<LabelData>(fragment.data))
        {
            auto labelData = std::get<LabelData>(fragment.data);
            if (auto it = labelTable.find(labelData.value); it != labelTable.end())
            {
                errors.push_back(GenerationError {
                    GenerationError::Type::LabelAlreadyDefined,
                    fragment.range,
                });
                continue;
            }

            Address address { base, *pNumWordsToIncrement * 4 };
            labelTable.insert(std::make_pair(labelData.value, address));
        }
        else if (std::holds_alternative<LAFormatData>(fragment.data))
        {
            auto const& formatData = std::get<LAFormatData>(fragment.data);

            // Assumptions:
            // 1. .data segment always comes first
            // 2. an address pointing to .text segment cannot be the operand of a LA format
            // instruction.
            if (auto it = labelTable.find(formatData.target);
                it != labelTable.end() && it->second.base == Address::BaseType::DataSegment)
            {
                if ((it->second.offset & 0xFFFF) == 0)
                    *pNumWordsToIncrement += 1;
                else
                    *pNumWordsToIncrement += 2;
            }
            else
            {
                // this contradicts to the assumption 2
                errors.push_back(GenerationError {
                    GenerationError::Type::OperandIsLabelInTextSegment,
                    fragment.range,
                });
                continue;
            }
        }
        else
        {
            *pNumWordsToIncrement += 1;
        }
    }

    return { numDataWords, numTextWords, std::move(labelTable), std::move(errors) };
}

GenerationResult GenerateCodeInternal(std::vector<Fragment> const& fragments,
                                      ScanResult const&            scanResult)
{
    std::vector<GenerationError> errors;
    std::vector<uint32_t>        data;
    std::vector<uint32_t>        text;
    std::vector<uint32_t>*       pTarget = &text;

    data.reserve(scanResult.numDataWords);
    text.reserve(scanResult.numTextWords);

    LabelTable const& labelTable = scanResult.labelTable;

    Address  dataAddress { Address::BaseType::DataSegment, 0 };
    Address  textAddress { Address::BaseType::TextSegment, 0 };
    Address* pTargetAddress = &textAddress;

    for (auto& fragment : fragments)
    {
        if (std::holds_alternative<DataDirData>(fragment.data))
        {
            pTarget        = &data;
            pTargetAddress = &dataAddress;
        }
        else if (std::holds_alternative<TextDirData>(fragment.data))
        {
            pTarget        = &text;
            pTargetAddress = &textAddress;
        }
        else if (std::holds_alternative<WordDirData>(fragment.data))
        {
            auto wordDirData = std::get<WordDirData>(fragment.data);
            pTarget->push_back(wordDirData.value);
            pTargetAddress->MoveToNext();
        }
        else if (std::holds_alternative<LabelData>(fragment.data))
            /* Do nothing */;
        else if (std::holds_alternative<RFormatData>(fragment.data))
        {
            auto data = std::get<RFormatData>(fragment.data);

            InstrConverter<RFormatInstr> instr;
            instr->source1     = data.source1;
            instr->source2     = data.source2;
            instr->destination = data.destination;
            instr->function    = static_cast<uint32_t>(data.function);

            pTarget->push_back(instr);
            pTargetAddress->MoveToNext();
        }
        else if (std::holds_alternative<JRFormatData>(fragment.data))
        {
            auto data = std::get<JRFormatData>(fragment.data);

            InstrConverter<JRFormatInstr> instr;
            instr->source   = data.source;
            instr->function = static_cast<uint32_t>(data.function);

            pTarget->push_back(instr);
            pTargetAddress->MoveToNext();
        }
        else if (std::holds_alternative<SRFormatData>(fragment.data))
        {
            auto data = std::get<SRFormatData>(fragment.data);

            InstrConverter<SRFormatInstr> instr;
            instr->source      = data.source;
            instr->destination = data.destination;
            instr->shiftAmount = data.shiftAmount;
            instr->function    = static_cast<uint32_t>(data.function);

            pTarget->push_back(instr);
            pTargetAddress->MoveToNext();
        }
        else if (std::holds_alternative<IFormatData>(fragment.data))
        {
            auto data = std::get<IFormatData>(fragment.data);

            InstrConverter<IFormatInstr> instr;
            instr->operation   = static_cast<uint32_t>(data.operation);
            instr->source      = data.source;
            instr->destination = data.destination;
            instr->immediate   = data.immediate;

            pTarget->push_back(instr);
            pTargetAddress->MoveToNext();
        }
        else if (std::holds_alternative<BIFormatData>(fragment.data))
        {
            auto const& data = std::get<BIFormatData>(fragment.data);

            auto it = labelTable.find(data.target);
            if (it == labelTable.end())
            {
                errors.push_back(GenerationError {
                    GenerationError::Type::UndefinedLabelName,
                    fragment.range,
                });
                continue;
            }

            uint32_t currentAddress = *pTargetAddress;
            uint32_t targetAddress  = it->second;

            auto difference = static_cast<int32_t>(targetAddress - currentAddress - 4);
            if (difference < static_cast<int32_t>(std::numeric_limits<int16_t>::min()) * 4
                || static_cast<int32_t>(std::numeric_limits<int16_t>::max()) * 4 < difference)
            {
                errors.push_back(GenerationError {
                    GenerationError::Type::BranchTargetTooFar,
                    fragment.range,
                });
                continue;
            }

            InstrConverter<BIFormatInstr> instr;
            instr->operation   = static_cast<uint32_t>(data.operation);
            instr->source      = data.source;
            instr->destination = data.destination;
            instr->offset      = difference / 4;

            pTarget->push_back(instr);
            pTargetAddress->MoveToNext();
        }
        else if (std::holds_alternative<IIFormatData>(fragment.data))
        {
            auto data = std::get<IIFormatData>(fragment.data);

            InstrConverter<IIFormatInstr> instr;
            instr->operation   = static_cast<uint32_t>(data.operation);
            instr->destination = data.destination;
            instr->immediate   = data.immediate;

            pTarget->push_back(instr);
            pTargetAddress->MoveToNext();
        }
        else if (std::holds_alternative<OIFormatData>(fragment.data))
        {
            auto data = std::get<OIFormatData>(fragment.data);

            InstrConverter<OIFormatInstr> instr;
            instr->operation = static_cast<uint32_t>(data.operation);
            instr->operand1  = data.operand1;
            instr->operand2  = data.operand2;
            instr->offset    = data.offset;

            pTarget->push_back(instr);
            pTargetAddress->MoveToNext();
        }
        else if (std::holds_alternative<JFormatData>(fragment.data))
        {
            auto const& data = std::get<JFormatData>(fragment.data);

            auto it = labelTable.find(data.target);
            if (it == labelTable.end())
            {
                errors.push_back(GenerationError {
                    GenerationError::Type::UndefinedLabelName,
                    fragment.range,
                });
                continue;
            }

            uint32_t targetAddress = it->second;
            if (targetAddress >= (1 << 28))
            {
                errors.push_back(GenerationError {
                    GenerationError::Type::JumpAddressTooBig,
                    fragment.range,
                });
                continue;
            }

            InstrConverter<JFormatInstr> instr;
            instr->operation = static_cast<uint32_t>(data.opeartion);
            instr->target    = (targetAddress >> 2);

            pTarget->push_back(instr);
            pTargetAddress->MoveToNext();
        }
        else /* if (std::holds_alternative<LAFormatData>(fragment.data) */
        {
            auto const& data = std::get<LAFormatData>(fragment.data);

            auto it = labelTable.find(data.target);
            if (it == labelTable.end())
            {
                errors.push_back(GenerationError {
                    GenerationError::Type::UndefinedLabelName,
                    fragment.range,
                });
                continue;
            }

            uint32_t targetAddress = it->second;
            if (targetAddress >= (1 << 16))
            {
                InstrConverter<IIFormatInstr> instr;
                instr->operation   = static_cast<uint32_t>(IIFormatOperation::LUI);
                instr->destination = data.destination;
                instr->immediate   = (targetAddress >> 16);

                pTarget->push_back(instr);
                pTargetAddress->MoveToNext();
            }
            targetAddress &= 0x00FF;

            if (targetAddress)
            {
                InstrConverter<IFormatInstr> instr;
                instr->operation   = static_cast<uint32_t>(IFormatOperation::ORI);
                instr->source      = data.destination;
                instr->destination = data.destination;
                instr->immediate   = targetAddress;

                pTarget->push_back(instr);
                pTargetAddress->MoveToNext();
            }
        }
    }

    if (!errors.empty())
        return CannotGenerate { std::move(errors) };
    else
        return CanGenerate { std::move(data), std::move(text) };
}

}

GenerationResult GenerateCode(std::vector<Fragment> const& fragments)
{
    ScanResult scanResult = ScanFragments(fragments);
    if (!scanResult.errors.empty())
        return CannotGenerate { std::move(scanResult.errors) };

    return GenerateCodeInternal(fragments, scanResult);
}
