// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef SIMPLE_MIPS_ASM_FILE
#define SIMPLE_MIPS_ASM_FILE

#include <simple-mips-asm/Generation.hh>

#include <filesystem>
#include <string>
#include <variant>

/// <summary>
/// Represents an error occurred when reading given files.
/// </summary>
struct FileReadError
{
    enum class Type
    {
        GivenPathIsDirectory,
        FileDoesNotExist
    };

    Type type;
};

struct CanRead
{
    std::string content;
};

struct CannotRead
{
    FileReadError error;
};

using FileReadResult = std::variant<CanRead, CannotRead>;

FileReadResult ReadFile(std::filesystem::path const& path);

/// <summary>
/// Represents an error occurred when writing given strings to files.
/// </summary>
struct FileWriteError
{
    enum class Type
    {
        GivenPathIsDirectory,
        CannotOpenFile,
    };

    Type type;
};

struct CanWrite
{};

struct CannotWrite
{
    FileWriteError error;
};

using FileWriteResult = std::variant<CanWrite, CannotWrite>;

FileWriteResult WriteFile(std::filesystem::path const& path, CanGenerate const& result);

#endif