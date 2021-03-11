// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef SIMPLE_MIPS_ASM_COMMON_HH
#define SIMPLE_MIPS_ASM_COMMON_HH

template <typename... VisitorTs>
struct VisitorList : VisitorTs...
{
    using VisitorTs::operator()...;
};

#endif