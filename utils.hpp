
#ifndef KRACKME_UTILS_H
#define KRACKME_UTILS_H

#include <triton/context.hpp>

using namespace triton;

#define debug_printf(format, ...)                                              \
{                                                                              \
    if (DEBUG)                                                                 \
	std::printf("\x1b[31m" format "\x1b[0m", __VA_ARGS__);                 \
}
#define debug_puts(str)                                                        \
{                                                                              \
    if (DEBUG)                                                                 \
	std::puts("\x1b[31m" str "\x1b[0m");                                   \
}
#define triton_printf(format, ...)                                             \
{                                                                              \
    std::printf("\x1b[33m" format "\x1b[0m", __VA_ARGS__);                     \
}
#define triton_puts(str)                                                       \
{                                                                              \
    std::puts("\x1b[33m" str "\x1b[0m");                                       \
}

uint64 getStack(int number);
void setStack(int number, const uint64 value);
arch::Register getArgReg(int number);
uint64 getArg(int number);
void setArg(int number, const uint64 value);
uint64 getGpr(const std::string &name);
void setGpr(const std::string &name, const uint64 value);
arch::register_e getGprId(const std::string &name);
uint64 allocate(uint8 *buf, uint64 size);
std::string toHex(uint64 ptr, uint32 size);
std::string readAsciiString(uint64 ptr, uint64 len);
std::string readUtf8String(uint64 ptr, uint64 len);
uint64 lenString(uint64 ptr);
uint64 printfArgAmount(std::string format);

#endif
