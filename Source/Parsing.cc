// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <simple-mips-asm/Formats.hh>
#include <simple-mips-asm/Parsing.hh>

#include <cctype>
#include <charconv>
#include <limits>
#include <string_view>
#include <unordered_map>

using namespace std::literals::string_view_literals;

namespace
{

// ----------------------------------------  Constants ----------------------------------------- //

constexpr uint8_t NumRegisters = 32;

// -----------------------------------  Parser output types ------------------------------------ //

using Iterator = std::vector<Token>::const_iterator;

/// <summary>
/// Returned when the parser cannot parse the given token stream.
/// </summary>
struct CannotParse
{
    ParsingError::Type errorType;
    Iterator           errorAt;
};

/// <summary>
/// Returned when the parser can parse the given token stream.
/// </summary>
struct CanParse
{
    FragmentData data;
    Iterator     fragmentEnd;
};

/// <summary>
/// Union of possible parser output types
/// </summary>
using ParserOutput = std::variant<CannotParse, CanParse>;

/// <summary>
/// Pattern matcher
/// </summary>
using Parser = ParserOutput (*)(Iterator, Iterator);

// ----------------------------------------  Utilities ----------------------------------------- //

struct CaseInsensitiveHash
{
    constexpr size_t operator()(std::string_view const& lhs) const
    {
        size_t rtn = _fnvOffsetBasis;

        for (char c : lhs)
        {
            rtn ^= size_t(toupper(c));
            rtn *= _fnvPrime;
        }

        return rtn;
    }

  private:
    constexpr static size_t _fnvOffsetBasis = 0xCBF29CE484222325;
    constexpr static size_t _fnvPrime       = 0x00000100000001B3;
};

struct CaseInsensitiveEqual
{
    constexpr bool operator()(std::string_view const& lhs, std::string_view const& rhs) const
    {
        return lhs.size() == rhs.size()
               && std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](char l, char r) {
                      return toupper(l) == toupper(r);
                  });
    }
};

template <typename LeftT, typename... ValueTs>
bool IsOneOf(LeftT left, ValueTs... valuesToCompare)
{
    return ((left == valuesToCompare) || ...);
}

template <typename T>
bool GetInteger(Iterator current, T& output)
{
    if (current->type == Token::Type::Integer)
    {
        auto stringValue      = current->value;
        auto stringValueBegin = stringValue.data();
        auto stringValueEnd   = stringValueBegin + stringValue.size();
        std::from_chars(stringValueBegin, stringValueEnd, output, 10);
    }
    else if (current->type == Token::Type::HexInteger)
    {
        auto stringValue      = current->value.substr(2);
        auto stringValueBegin = stringValue.data();
        auto stringValueEnd   = stringValueBegin + stringValue.size();
        std::from_chars(stringValueBegin, stringValueEnd, output, 16);
    }
    else
    {
        return false;
    }

    return true;
}

// ---------------------------------  Instruction name tables ---------------------------------- //

template <typename T>
using InstructionTable
    = std::unordered_map<std::string_view, T, CaseInsensitiveHash, CaseInsensitiveEqual>;

InstructionTable<RFormatFunction> const _rFormatTable {
    { "ADDU"sv, RFormatFunction::ADDU }, { "SUBU"sv, RFormatFunction::SUBU },
    { "AND"sv, RFormatFunction::AND },   { "OR"sv, RFormatFunction::OR },
    { "NOR"sv, RFormatFunction::NOR },   { "SLTU"sv, RFormatFunction::SLTU },
};

InstructionTable<JRFormatFunction> const _jrFormatTable {
    { "JR"sv, JRFormatFunction::JR },
};

InstructionTable<SRFormatFunction> const _srFormatTable {
    { "SLL"sv, SRFormatFunction::SLL },
    { "SRL"sv, SRFormatFunction::SRL },
};

InstructionTable<IFormatOperation> const _iFormatTable {
    { "ADDIU"sv, IFormatOperation::ADDIU },
    { "ANDI"sv, IFormatOperation::ANDI },
    { "ORI"sv, IFormatOperation::ORI },
    { "SLTIU"sv, IFormatOperation::SLTIU },
};

InstructionTable<BIFormatOperation> const _biFormatTable {
    { "BEQ"sv, BIFormatOperation::BEQ },
    { "BNE"sv, BIFormatOperation::BNE },
};

InstructionTable<IIFormatOperation> const _iiFormatTable {
    { "LUI"sv, IIFormatOperation::LUI },
};

InstructionTable<OIFormatOperation> const _oiFormatTable {
    { "LB"sv, OIFormatOperation::LB },
    { "LW"sv, OIFormatOperation::LW },
    { "SB"sv, OIFormatOperation::SB },
    { "SW"sv, OIFormatOperation::SW },
};

InstructionTable<JFormatOperation> const _jFormatTable {
    { "J"sv, JFormatOperation::J },
    { "JAL"sv, JFormatOperation::JAL },
};

InstructionTable<LAFormatType> const _laFormatTable {
    { "LA"sv, LAFormatType::LA },
};

// -----------------------------------------  Parsers ------------------------------------------ //

// Defines an iterator indicating the current position.
#define DEFINE_CURRENT Iterator current = begin

// Returns CannotParse with UnexpectedEof.
#define UNEXPECTED_EOF return CannotParse { ParsingError::Type::UnexpectedEof, current };

// Returns CannotParse with UnexpectedToken.
#define UNEXPECTED_TOKEN return CannotParse { ParsingError::Type::UnexpectedToken, current };

// Returns CannotParse with UnexpectedValue.
#define UNEXPECTED_VALUE return CannotParse { ParsingError::Type::UnexpectedValue, current };

// Returns CanParse with the given data.
#define RESULT(data) return CanParse { data, current == end ? current : current + 1 };

// Advances the iterator defined by DEFINE_CURRENT.
#define ADVANCE_CURRENT                                                                            \
    {                                                                                              \
        ++current;                                                                                 \
        if (current == end)                                                                        \
            return CannotParse { ParsingError::Type::UnexpectedEof, current };                     \
    }

// Advances the iterator until non-whitespace token appears.
#define SKIP_WHITESPACES                                                                           \
    while (current->type == Token::Type::Whitespace) ADVANCE_CURRENT

// Advances the iterator until non-whitespace token appears. If that first non-whitespace token's
// type is not the expected type, returns CannotParse with UnexpectedToken.
#define EXPECT_NEXT(...)                                                                           \
    SKIP_WHITESPACES;                                                                              \
    if (!IsOneOf(current->type, __VA_ARGS__))                                                      \
    UNEXPECTED_TOKEN

// Advances the iterator until non-whitespace token appears. If that first non-whitespace token is
// not a Word with the given value, returns CannotParse with UnexpectedValue.
#define EXPECT_WORD(ExpectedValue)                                                                 \
    EXPECT_NEXT(Token::Type::Word);                                                                \
    if (!CaseInsensitiveEqual {}(current->value, ExpectedValue))                                   \
    UNEXPECTED_VALUE

// Advances the iterator until non-whitespace token appears. If that first non-whitespace token is
// not a Word where the value is registered in the given table, returns CannotParse with
// UnexpectedValue. This macro must be used only once within one function.
#define EXPECT_OPCODE(Table)                                                                       \
    EXPECT_NEXT(Token::Type::Word);                                                                \
    auto it = Table.find(current->value);                                                          \
    if (it == Table.end())                                                                         \
        UNEXPECTED_VALUE;                                                                          \
    auto type = it->second;

// Checks whether the next incoming tokens indicate a register.
#define EXPECT_REGISTER(OutputVariableName)                                                        \
    EXPECT_NEXT(Token::Type::Dollar);                                                              \
    ADVANCE_CURRENT;                                                                               \
    EXPECT_NEXT(Token::Type::Integer);                                                             \
    uint8_t OutputVariableName;                                                                    \
    if (!GetInteger(current, OutputVariableName) || NumRegisters <= OutputVariableName)            \
        UNEXPECTED_VALUE;

// Checks whether the next incoming token indicates an immediate number.
#define EXPECT_IMM(OutputVariableName)                                                             \
    EXPECT_NEXT(Token::Type::Integer, Token::Type::HexInteger);                                    \
    int64_t OutputVariableName;                                                                    \
    if (!GetInteger(current, OutputVariableName))                                                  \
        UNEXPECTED_VALUE;

// Checks whether the next incoming token is a new line character or EOF.
#define EXPECT_NEW_LINE_OR_EOF                                                                     \
    while (current != end && current->type == Token::Type::Whitespace) ++current;                  \
    if (current != end && current->type != Token::Type::NewLine)                                   \
    UNEXPECTED_TOKEN

// DataDirective: Dot + "data"
ParserOutput Data(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_NEXT(Token::Type::Dot);
    ADVANCE_CURRENT;
    EXPECT_WORD("data");

    RESULT(DataDirData {});
}

// TextDirective: Dot + "text"
ParserOutput Text(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_NEXT(Token::Type::Dot);
    ADVANCE_CURRENT;
    EXPECT_WORD("text");

    RESULT(TextDirData {});
}

// WordDirective: Dot + "word" + (Integer | HexInteger) + (NewLine | EOF)
ParserOutput Word(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_NEXT(Token::Type::Dot);
    ADVANCE_CURRENT;
    EXPECT_WORD("word");
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Integer, Token::Type::HexInteger);
    uint32_t result;
    if (!GetInteger<uint32_t>(current, result))
        UNEXPECTED_VALUE;
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT(WordDirData { result });
}

// Label: Word + Colon
ParserOutput Label(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_NEXT(Token::Type::Word);
    auto labelName = current->value;
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Colon);

    RESULT(LabelData { labelName });
}

// RFormatInstruction: RFormatOpcode + Register + Register + Register + (NewLine | EOF)
ParserOutput RFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_rFormatTable);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(destination);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(source1);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(source2);
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((RFormatData { type, destination, source1, source2 }));
}

// JRFormatInstruction: JRFormatOpcode + Register + (NewLine | EOF)
ParserOutput JRFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_jrFormatTable);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(source);
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((JRFormatData { type, source }));
}

// SRFormatInstruction: SRFormatOpcode + Register + Register + (Integer | HexInteger) + (NewLine |
// EOF)
ParserOutput SRFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_srFormatTable);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(destination);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(source);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_IMM(shiftAmount);
    if (shiftAmount < 0 || 32 <= shiftAmount)
        UNEXPECTED_VALUE;
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((SRFormatData { type, destination, source, static_cast<uint8_t>(shiftAmount) }));
}

// IFormatInstruction: IFormatOpcode + Register + Register + (Integer | HexInteger) + (NewLine |
// EOF)
ParserOutput IFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_iFormatTable);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(destination);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(source);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_IMM(immediate);
    if (immediate < static_cast<int64_t>(std::numeric_limits<int16_t>::min())
        || static_cast<int64_t>(std::numeric_limits<uint16_t>::max()) < immediate)
        UNEXPECTED_VALUE;
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((IFormatData { type, destination, source, static_cast<uint16_t>(immediate) }));
}

// BIFormatInstruction: BIFormatOpcode + Register + Register + Word + (NewLine | EOF)
ParserOutput BIFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_biFormatTable);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(source);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(destination);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Word);
    auto target = current->value;
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((BIFormatData { type, source, destination, target }));
}

// IIFormatInstruction: IIFormatOpcode + Register + (Integer | HexInteger) + (NewLine | EOF)
ParserOutput IIFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_iiFormatTable);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(destination);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_IMM(immediate);
    if (immediate < static_cast<int64_t>(std::numeric_limits<int16_t>::min())
        || static_cast<int64_t>(std::numeric_limits<uint16_t>::max()) < immediate)
        UNEXPECTED_VALUE;
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((IIFormatData { type, destination, static_cast<uint16_t>(immediate) }));
}

// OIFormatInstruction: OIFormatOpcode + Register + (Integer | HexInteger) + BracketOpen + Register
// + BracketClose + (NewLine | EOF)
ParserOutput OIFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_oiFormatTable);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(operand2);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_IMM(offset);
    if (offset < static_cast<int64_t>(std::numeric_limits<int16_t>::min())
        || static_cast<int64_t>(std::numeric_limits<uint16_t>::max()) < offset)
        UNEXPECTED_VALUE;
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::BracketOpen);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(operand1);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::BracketClose);
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((OIFormatData { type, operand2, static_cast<uint16_t>(offset), operand1 }));
}

// JFormatInstruction: JFormatOpcode + Word + (NewLine | EOF)
ParserOutput JFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_jFormatTable);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Word);
    auto target = current->value;
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((JFormatData { type, target }));
}

// LAFormatInstruction: LAFormatOpcode + Word + (NewLine | EOF)
ParserOutput LAFormatInstruction(Iterator begin, Iterator end)
{
    DEFINE_CURRENT;

    EXPECT_OPCODE(_laFormatTable);
    ADVANCE_CURRENT;
    EXPECT_REGISTER(destination);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Comma);
    ADVANCE_CURRENT;
    EXPECT_NEXT(Token::Type::Word);
    auto target = current->value;
    ADVANCE_CURRENT;
    EXPECT_NEW_LINE_OR_EOF;

    RESULT((LAFormatData { type, destination, target }));
}

Parser _parsers[] = {
    Data,
    Text,
    Word,
    Label,
    RFormatInstruction,
    JRFormatInstruction,
    SRFormatInstruction,
    IFormatInstruction,
    BIFormatInstruction,
    IIFormatInstruction,
    OIFormatInstruction,
    JFormatInstruction,
    LAFormatInstruction,
};

Iterator SkipEmptyLine(Iterator begin, Iterator end)
{
    Iterator current = begin;
    while (current != end && current->type == Token::Type::Whitespace) ++current;
    if (current != end && current->type != Token::Type::NewLine)
        return begin;
    return current == end ? current : current + 1;
}

}

ParseResult Parse(std::vector<Token> const& tokens)
{
    std::vector<Fragment>     fragments;
    std::vector<ParsingError> errors;

    auto       begin = tokens.begin();
    auto const end   = tokens.end();

    while (begin != end)
    {
        ParsingError::Type errorType;
        Iterator           maxErrorAt = begin;
        bool               parsed     = false;
        for (auto parser : _parsers)
        {
            auto result = parser(begin, end);
            if (std::holds_alternative<CanParse>(result))
            {
                auto output = std::get<CanParse>(result);
                fragments.push_back(
                    { output.data, { begin->range.begin, output.fragmentEnd[-1].range.end } });
                begin  = output.fragmentEnd;
                parsed = true;
                break;
            }
            else /* if (std::holds_alternative<CannotParse>(result)) */
            {
                auto output = std::get<CannotParse>(result);
                if (maxErrorAt <= output.errorAt)
                {
                    maxErrorAt = output.errorAt;
                    errorType  = output.errorType;
                }
            }
        }

        if (!parsed)
        {
            auto emptyLineSkipResult = SkipEmptyLine(begin, end);
            if (emptyLineSkipResult == begin)
            {
                errors.push_back({ errorType, maxErrorAt->range });
                begin = maxErrorAt + 1;
            }
            else
                begin = emptyLineSkipResult;
        }
    }

    return { std::move(fragments), std::move(errors) };
}
