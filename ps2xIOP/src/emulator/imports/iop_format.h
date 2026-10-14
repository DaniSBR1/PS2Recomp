#pragma once

#include "../core/iop_cpu.h"
#include "../core/iop_memory.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace ps2x::iop::detail
{
    // printf-style formatter shared by stdio (printf) and sysclib (sprintf/vsprintf).
    // nextArg() returns the next 32-bit variadic argument; the IOP has no FPU, so no %f.
    template <typename NextArg>
    std::string formatIopString(const IopMemory &memory, const std::string &format, NextArg nextArg)
    {
        std::string out;
        for (size_t i = 0; i < format.size(); ++i)
        {
            if (format[i] != '%')
            {
                out.push_back(format[i]);
                continue;
            }
            std::string spec = "%";
            size_t j = i + 1;
            while (j < format.size() && std::strchr("-+ #0123456789.lhz", format[j]) != nullptr)
            {
                if (format[j] != 'l' && format[j] != 'h' && format[j] != 'z')
                    spec.push_back(format[j]);
                ++j;
            }
            if (j >= format.size())
                break;
            const char conv = format[j];
            char buffer[512];
            switch (conv)
            {
            case '%':
                out.push_back('%');
                break;
            case 'd':
            case 'i':
                std::snprintf(buffer, sizeof(buffer), (spec + "d").c_str(), static_cast<int32_t>(nextArg()));
                out += buffer;
                break;
            case 'u':
            case 'x':
            case 'X':
            case 'o':
            case 'c':
                std::snprintf(buffer, sizeof(buffer), (spec + conv).c_str(), nextArg());
                out += buffer;
                break;
            case 'p':
                std::snprintf(buffer, sizeof(buffer), "0x%08X", nextArg());
                out += buffer;
                break;
            case 's':
                std::snprintf(buffer, sizeof(buffer), (spec + "s").c_str(), memory.readString(nextArg(), 256u).c_str());
                out += buffer;
                break;
            default:
                out += spec;
                out.push_back(conv);
                break;
            }
            i = j;
        }
        return out;
    }

    // Variadic arguments in registers (MIPS o32 ABI): they start in firstArgReg, continue up to $a3
    // and then on the stack (argument n at sp + 4 * n).
    inline std::string formatIopStringFromRegisters(const IopMemory &memory, const IopCpuState &cpu,
                                                    const std::string &format, uint32_t firstArgReg)
    {
        uint32_t reg = firstArgReg;
        return formatIopString(memory, format, [&]() -> uint32_t
                               {
                                   const uint32_t value = reg <= 7u ? cpu.gpr[reg]
                                                                    : memory.read32(cpu.gpr[29] + (reg - 4u) * 4u);
                                   ++reg;
                                   return value; });
    }

    // va_list (vsprintf): on the IOP it is a pointer to consecutive 32-bit arguments.
    inline std::string formatIopStringFromVaList(const IopMemory &memory, const std::string &format, uint32_t vaList)
    {
        return formatIopString(memory, format, [&]() -> uint32_t
                               {
                                   const uint32_t value = memory.read32(vaList);
                                   vaList += 4u;
                                   return value; });
    }
}
