// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <simple-mips-asm/File.hh>
#include <simple-mips-asm/Generation.hh>
#include <simple-mips-asm/Parsing.hh>
#include <simple-mips-asm/Tokenization.hh>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace std::literals::string_literals;

namespace
{

#define CASE(ErrorTypename, ErrorType)                                                             \
    case ErrorTypename::Type::ErrorType: std::cerr << #ErrorType; break

std::ostream& operator<<(std::ostream& os, Position const& position)
{
    return os << position.line << ',' << position.character;
}

std::ostream& operator<<(std::ostream& os, Range const& range)
{
    return os << '(' << range.begin << ';' << range.end << ')';
}

void ReportFileReadError(char const* inputPath, FileReadError error)
{
    std::cerr << inputPath << ": FileReadError: ";
    switch (error.type)
    {
        CASE(FileReadError, GivenPathIsDirectory);
        CASE(FileReadError, FileDoesNotExist);
    }
    std::cerr << std::endl;
}

void ReportTokenizationErrors(char const* inputPath, std::vector<TokenizationError> const& errors)
{
    for (auto const& error : errors)
    {
        std::cerr << inputPath << error.range << ": TokenizationError: ";
        switch (error.type)
        {
            CASE(TokenizationError, InvalidCharacter);
            CASE(TokenizationError, InvalidFormat);
        }
        std::cerr << std::endl;
    }
}

void ReportParsingErrors(char const* inputPath, std::vector<ParsingError> const& errors)
{
    for (auto const& error : errors)
    {
        std::cerr << inputPath << error.range << ": ParsingError: ";
        switch (error.type)
        {
            CASE(ParsingError, UnexpectedToken);
            CASE(ParsingError, UnexpectedEof);
            CASE(ParsingError, UnexpectedValue);
        }
        std::cerr << std::endl;
    }
}

void ReportGenerationErrors(char const* inputPath, std::vector<GenerationError> const& errors)
{
    for (auto const& error : errors)
    {
        std::cerr << inputPath << error.range << ": GenerationError: ";
        switch (error.type)
        {
            CASE(GenerationError, UndefinedLabelName);
            CASE(GenerationError, OperandIsLabelInTextSegment);
            CASE(GenerationError, LabelAlreadyDefined);
            CASE(GenerationError, BranchTargetTooFar);
            CASE(GenerationError, JumpAddressTooBig);
        }
        std::cerr << std::endl;
    }
}

void ReportFileWriteError(fs::path const& outputPath, FileWriteError error)
{
    std::cerr << outputPath << ": FileWriteError: ";
    switch (error.type)
    {
        CASE(FileWriteError, GivenPathIsDirectory);
        CASE(FileWriteError, CannotOpenFile);
    }
    std::cerr << std::endl;
}

void ReportBadAlloc(char const* inputPath)
{
    std::cerr << inputPath << ": BadAlloc" << std::endl;
}

void HandleFile(char const* inputPath) noexcept
{
    try
    {
        // read the given file
        auto fileReadResult = ReadFile(inputPath);
        if (std::holds_alternative<CannotRead>(fileReadResult))
            return ReportFileReadError(inputPath, std::get<CannotRead>(fileReadResult).error);
        auto const& file = std::get<CanRead>(fileReadResult).content;

        // tokenize source
        auto tokenizationResult = Tokenize(file);
        if (auto const& errors = tokenizationResult.errors; !errors.empty())
            return ReportTokenizationErrors(inputPath, errors);
        auto const& tokens = tokenizationResult.tokens;

        // parse tokens
        auto parseResult = Parse(tokens);
        if (auto const& errors = parseResult.errors; !errors.empty())
            return ReportParsingErrors(inputPath, errors);
        auto const& fragments = parseResult.fragments;

        // generate machine code
        auto generationResult = GenerateCode(fragments);
        if (std::holds_alternative<CannotGenerate>(generationResult))
            return ReportGenerationErrors(inputPath,
                                          std::get<CannotGenerate>(generationResult).errors);
        auto const& code = std::get<CanGenerate>(generationResult);

        // write code to file
        fs::path outputPath = inputPath;
        outputPath.replace_extension(".o");
        auto fileWriteResult = WriteFile(outputPath, code);
        if (std::holds_alternative<CannotWrite>(fileWriteResult))
            return ReportFileWriteError(outputPath, std::get<CannotWrite>(fileWriteResult).error);

        std::cerr << inputPath << " -> " << outputPath << std::endl;
    }
    catch (std::bad_alloc const&)
    {
        ReportBadAlloc(inputPath);
    }
}

}

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false);
    for (int i = 1; i < argc; ++i) HandleFile(argv[i]);
}
