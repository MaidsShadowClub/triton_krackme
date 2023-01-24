#include <triton/context.hpp>

using namespace triton;

#define debug(format, ...) {if (DEBUG) printf(format, __VA_ARGS__);}

uint64 getStack(int number);
void setStack(int number, const uint64 value);
uint64 getArg(int number);
void setArg(int number, const uint64 value);
uint64 getGpr(std::string name);
void setGpr(std::string name, const uint64 value);
uint64 allocate(uint8 *buf, uint64 size);
std::string toHex(uint64 ptr, uint32 size);
std::string readAsciiString(uint64 ptr, uint64 len);
std::string readUtf8String(uint64 ptr, uint64 len);
uint64 lenString(uint64 ptr);
uint64 printfArgAmount(std::string format);
arch::register_e getGprId(std::string name);
