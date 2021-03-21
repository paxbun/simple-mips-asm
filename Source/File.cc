// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <simple-mips-asm/File.hh>

#include <fstream>
#include <iomanip>

namespace fs = std::filesystem;

namespace
{

std::ostream& PrintWord(std::ostream& os, uint32_t word)
{
    return os << "0x" << std::hex << std::setfill('0') << std::setw(8) << word << '\n';
}

}

FileReadResult ReadFile(std::filesystem::path const& path)
{
    if (fs::is_directory(path))
        return CannotRead { FileReadError::Type::GivenPathIsDirectory };

    std::ifstream ifs { path };
    if (!ifs)
        return CannotRead { FileReadError::Type::FileDoesNotExist };

    ifs.seekg(0, std::ios::end);
    size_t fileSize = ifs.tellg();

    std::string content(fileSize, ' ');
    ifs.seekg(0);
    ifs.read(std::addressof(content[0]), fileSize);

    return CanRead { std::move(content) };
}

FileWriteResult WriteFile(std::filesystem::path const& path, CanGenerate const& result)
{
    if (fs::is_directory(path))
        return CannotWrite { FileWriteError::Type::GivenPathIsDirectory };

    std::ofstream ofs { path };
    if (!ofs)
        return CannotWrite { FileWriteError::Type::CannotOpenFile };

    ofs << std::uppercase;

    PrintWord(ofs, static_cast<uint32_t>(result.text.size()));
    PrintWord(ofs, static_cast<uint32_t>(result.data.size()));

    for (uint32_t word : result.text) PrintWord(ofs, word);
    for (uint32_t word : result.data) PrintWord(ofs, word);
}
