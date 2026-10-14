#include "iop_stdio.h"

#include "../core/iop_cpu.h"
#include "../core/iop_memory.h"
#include "ps2x/iop/iop_host.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace ps2x::iop::detail
{
    namespace
    {
        // Formats an IOP printf (MIPS o32 ABI): variadic arguments start in register firstArgReg
        // ($a1 for printf), continue in $a2/$a3 and then on the stack (argument n at sp + 4 * n).
        std::string formatIopPrintf(const IopMemory &memory, const IopCpuState &cpu, const std::string &format, uint32_t firstArgReg)
        {
            uint32_t argIndex = 0u;
            const auto nextArg = [&]() -> uint32_t
            {
                const uint32_t reg = firstArgReg + argIndex;
                const uint32_t value = reg <= 7u ? cpu.gpr[reg]
                                                 : memory.read32(cpu.gpr[29] + 16u + (reg - 8u + 4u) * 4u - 16u);
                ++argIndex;
                return value;
            };
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
    }

    IopStdio::IopStdio(IopHost &host, IopMemory &memory) noexcept
        : m_host(host), m_memory(memory)
    {
    }

    bool IopStdio::dispatchImport(uint16_t ordinal, IopCpuState &cpu)
    {
        const uint32_t a0 = cpu.gpr[4];
        const uint32_t a1 = cpu.gpr[5];
        const auto setV0 = [&](uint32_t value)
        {
            cpu.gpr[2] = value;
        };
        const auto logString = [&](std::string_view prefix, uint32_t address, uint32_t resultBias = 0u)
        {
            const std::string text = m_memory.readString(address, 2048u);
            m_host.log(LogLevel::Info, std::string(prefix) + text);
            setV0(static_cast<uint32_t>(text.size()) + resultBias);
        };

        switch (ordinal)
        {
        case 4: // printf
        {
            const std::string text = formatIopPrintf(m_memory, cpu, m_memory.readString(a0, 2048u), 5u);
            m_host.log(LogLevel::Info, std::string("[IOP printf] ") + text);
            setV0(static_cast<uint32_t>(text.size()));
            return true;
        }
        case 5: // getchar
        case 10:
            setV0(0xFFFFFFFFu);
            return true;
        case 6: // putchar
            m_host.log(LogLevel::Info, std::string("[IOP putchar] ") + static_cast<char>(a0 & 0xFFu));
            setV0(a0 & 0xFFu);
            return true;
        case 7: // puts
            logString("[IOP puts] ", a0, 1u);
            return true;
        case 8: // gets
        case 13:
            setV0(0u);
            return true;
        case 9: // fdprintf
            logString("[IOP fdprintf] ", a1);
            return true;
        case 11:
            setV0(a0 & 0xFFu);
            return true;
        case 12: // fdputs
            logString("[IOP fdputs] ", a0);
            return true;
        case 14: // vfdprintf
            logString("[IOP vfdprintf] ", a1);
            return true;
        default:
            return false;
        }
    }
}
