#include <cstdarg>
#include <regex>
#include <triton/context.hpp>

#include "utils.hpp"

using namespace triton;

extern triton::Context gctx;
extern bool DEBUG;

uint64 heap_base = 0xAFFFFFFF;

std::map<int, std::vector<std::pair<std::string, arch::register_e>>> gpr = {
    {arch::ARCH_X86_64,
     {{"ip", arch::ID_REG_X86_RIP},
      {"sp", arch::ID_REG_X86_RSP},
      {"bp", arch::ID_REG_X86_RBP},
      {"ret", arch::ID_REG_X86_RAX}}},
    {arch::ARCH_X86,
     {{"ip", arch::ID_REG_X86_EIP},
      {"sp", arch::ID_REG_X86_ESP},
      {"bp", arch::ID_REG_X86_EBP},
      {"ret", arch::ID_REG_X86_EAX}}},
    {arch::ARCH_AARCH64,
     {{"ip", arch::ID_REG_AARCH64_PC},
      {"sp", arch::ID_REG_AARCH64_SP},
      {"bp", arch::ID_REG_AARCH64_X29},
      {"ret", arch::ID_REG_AARCH64_X0}}},
    {arch::ARCH_ARM32,
     {{"ip", arch::ID_REG_ARM32_PC},
      {"sp", arch::ID_REG_ARM32_SP},
      {"bp", arch::ID_REG_ARM32_R11},
      {"ret", arch::ID_REG_ARM32_R0}}}};

std::map<int, std::vector<arch::register_e>> arg_regs = {
    {arch::ARCH_X86_64,
     {arch::ID_REG_X86_RDI, arch::ID_REG_X86_RSI, arch::ID_REG_X86_RDX,
      arch::ID_REG_X86_R10, arch::ID_REG_X86_R8,
      arch::ID_REG_X86_R9}}, // depend on calling convention R10 can be replaced
                             // by RXC
    {arch::ARCH_X86,
     {arch::ID_REG_X86_EBX, arch::ID_REG_X86_ECX, arch::ID_REG_X86_EDX,
      arch::ID_REG_X86_ESI, arch::ID_REG_X86_EDI, arch::ID_REG_X86_EBX}},
    {arch::ARCH_AARCH64,
     {arch::ID_REG_AARCH64_X0, arch::ID_REG_AARCH64_X1, arch::ID_REG_AARCH64_X2,
      arch::ID_REG_AARCH64_X3, arch::ID_REG_AARCH64_X4,
      arch::ID_REG_AARCH64_X5}},
    {arch::ARCH_ARM32,
     {arch::ID_REG_ARM32_R0, arch::ID_REG_ARM32_R1, arch::ID_REG_ARM32_R2,
      arch::ID_REG_ARM32_R3, arch::ID_REG_ARM32_R4, arch::ID_REG_ARM32_R5}}};

uint64 getStack(int number) {
  auto sp_val = getGpr("sp");
  arch::MemoryAccess var(sp_val + number * gctx.getGprSize(),
                         gctx.getGprSize());
  return static_cast<uint64>(gctx.getConcreteMemoryValue(var));
}

void setStack(int number, const uint64 value) {
  auto sp_val = getGpr("sp");
  arch::MemoryAccess var(sp_val + number * gctx.getGprSize(),
                         gctx.getGprSize());
  gctx.setConcreteMemoryValue(var, value);
}

arch::Register getArgReg(int number) {
  auto regs = arg_regs[gctx.getArchitecture()];
  arch::Register reg;
  reg = gctx.getRegister(regs[number]);
  return reg;
}

uint64 getArg(int number) {
  auto regs = arg_regs[gctx.getArchitecture()];
  uint64 ret;
  arch::Register reg;
  switch (number) {
  case 0 ... 5:
    reg = gctx.getRegister(regs[number]);
    ret = static_cast<uint64>(gctx.getConcreteRegisterValue(reg));
    break;
  default:
    ret = getStack(number + 2); // stack is ip+old_sp+arg1+arg2+...
  }
  return ret;
}

void setArg(int number, const uint64 value) {
  auto regs = arg_regs[gctx.getArchitecture()];
  uint64 ret;
  arch::Register reg;
  switch (number) {
  case 0 ... 5:
    reg = gctx.getRegister(regs[number]);
    gctx.setConcreteRegisterValue(reg, value);
    break;
  default:
    setStack(number + 2, value); // stack is ip+old_sp+arg1+arg2+...
  }
}

uint64 getGpr(const std::string &name) {
  auto gpr_regs = gpr[gctx.getArchitecture()];
  auto reg_id = std::find_if(gpr_regs.begin(), gpr_regs.end(),
                             [=](auto r) { return r.first == name; });
  if (reg_id == gpr_regs.end())
    throw std::invalid_argument("cannot get this general purpose register");
  auto reg = gctx.getRegister(reg_id->second);
  return static_cast<uint64>(gctx.getConcreteRegisterValue(reg));
}

void setGpr(const std::string &name, const uint64 value) {
  auto gpr_regs = gpr[gctx.getArchitecture()];
  auto reg_id = std::find_if(gpr_regs.begin(), gpr_regs.end(),
                             [=](auto r) { return r.first == name; });
  if (reg_id == gpr_regs.end())
    throw std::invalid_argument("cannot set this general purpose register");
  auto reg = gctx.getRegister(reg_id->second);
  gctx.setConcreteRegisterValue(reg, value);
}

arch::register_e getGprId(const std::string &name) {
  auto gpr_regs = gpr[gctx.getArchitecture()];
  auto reg_id = std::find_if(gpr_regs.begin(), gpr_regs.end(),
                             [=](auto r) { return r.first == name; });
  if (reg_id == gpr_regs.end())
    throw std::invalid_argument("cannot get this general purpose register id");
  return reg_id->second;
}

uint64 allocate(uint8 *buf, uint64 size) {
  gctx.setConcreteMemoryAreaValue(heap_base, buf, size);
  uint64 ret = heap_base;
  heap_base += size;
  return ret;
}

std::string toHex(uint64 ptr, uint32 size) {
  char hex[4];
  std::string res = "";
  for (uint32 i = 0; i < size; i++) {
    snprintf(hex, 3, "%02x ", gctx.getConcreteMemoryValue(ptr + i));
    res += hex;
  }
  return res;
}

std::string readAsciiString(uint64 ptr, uint64 len) {
  std::string res = "";
  for (uint64 i = 0; i < len; i++) {
    uint8 c = gctx.getConcreteMemoryValue(ptr + i);
    if ((c < 0x20) || (c > 0x7F))
      break;
    res += c;
  }
  return res;
}

std::string readUtf8String(uint64 ptr, uint64 len) {
  std::string res = "";
  for (uint64 i = 0; i < len; i++) {
    uint8 c = gctx.getConcreteMemoryValue(ptr + i);
    if (c == 0)
      break;
    res += c;
  }
  return res;
}

uint64 lenString(uint64 ptr) {
  uint64 res;
  for (res = 0;; res++) {
    uint8 c = gctx.getConcreteMemoryValue(ptr + res);
    if (c == 0)
      break;
  }
  return res;
}

// print stuff

uint64 printfArgAmount(const std::string &format) {
  std::regex const re("%[0-9$#xXzldos]*");
  std::ptrdiff_t const count(
      std::distance(std::sregex_iterator(format.begin(), format.end(), re),
                    std::sregex_iterator()));
  return count;
}
