#include <LIEF/ELF.hpp>
#include <exception>
#include <memory>
#include <stdexcept>
#include <triton/archEnums.hpp>
#include <triton/callbacks.hpp>
#include <triton/callbacksEnums.hpp>
#include <triton/context.hpp>
#include <triton/cpuSize.hpp>
#include <triton/exceptions.hpp>
#include <triton/instruction.hpp>
#include <triton/memoryAccess.hpp>
#include <triton/modesEnums.hpp>
#include <triton/register.hpp>

#include "hooks.hpp"
#include "utils.hpp"

using namespace triton;

triton::Context gctx;
bool DEBUG = true;
std::unique_ptr<const LIEF::ELF::Binary> bin;

#define RELOC_BASE 0x10000000
#define NULL_HANDLER                                                           \
  {                                                                            \
    0, { "stub", NULL }                                                        \
  }
#define HANDLER(name)                                                          \
  {                                                                            \
    RELOC_BASE + __COUNTER__ *size::dword, {                                   \
      (std::string) #name, (uint64(*)()) & name##_HANDLER                      \
    }                                                                          \
  }
// __COUNTER__ used to determine hook-function in callback

struct hook_info {
  std::string name;
  uint64 (*func)();
};

// add relocation or symbol you would like to hook
std::map<uint64, hook_info> hook_rules{HANDLER(__libc_start_main),
                                       HANDLER(printf),
                                       HANDLER(puts),
                                       HANDLER(fflush),
                                       HANDLER(getlogin),
                                       HANDLER(usleep),
                                       HANDLER(putchar),
                                       HANDLER(exit),
                                       HANDLER(strlen),
                                       HANDLER(fgets),
                                       NULL_HANDLER};

void handleHooks() {
  auto pc = getGpr("ip");
  if (hook_rules.count(pc) == 0)
    return;

  auto rel = hook_rules[pc];
  if (rel.func == 0) {
    throw exceptions::Callbacks("trying to execute NULL_HANDLER");
  }
  uint64_t ret = rel.func();
  ret &= (~0 & (bitsize::qword - 1));
  setGpr("ret", ret);

  if (rel.name == "__libc_start_main") {
    return; // the function set own ip to able use argc, argv & env
  }

  // emulate ip
  setGpr("ip", getStack(0));

  // restore sp
  setGpr("sp", getStack(1));
}

void patchExection() {
  printf("Base: %08x\n", bin->imagebase());
  for (const LIEF::Relocation rel : bin->relocations()) {
    auto addr = rel.address();
    auto symb = bin->get_relocation(addr)->symbol();
    if (symb == nullptr)
      continue;

    auto name = symb->name();
    auto check_hook = [=](std::pair<uint64, hook_info> i) {
      return i.second.name == name;
    };
    auto hook = std::find_if(hook_rules.begin(), hook_rules.end(), check_hook);
    if (hook == hook_rules.end())
      continue;

    printf("[i] Replacing reloc %s\n", name.data());
    arch::MemoryAccess mem(addr, gctx.getGprSize());
    gctx.setConcreteMemoryValue(mem, hook->first);
  }
  for (const LIEF::Symbol symb : bin->symbols()) {
    auto addr = symb.value();
    if (addr == 0)
      continue;

    auto name = symb.name();
    auto check_hook = [=](std::pair<uint64, hook_info> i) {
      return i.second.name == name;
    };
    auto hook = std::find_if(hook_rules.begin(), hook_rules.end(), check_hook);
    if (hook == hook_rules.end())
      continue;

    printf("[i] Replacing symb %s\n", name.data());
    arch::MemoryAccess mem(addr, gctx.getGprSize());
    gctx.setConcreteMemoryValue(mem, hook->first);
  }
}

uint64_t loadExec(std::string str) {
  bin = LIEF::ELF::Parser::parse(str);
  if (bin == nullptr)
    throw std::invalid_argument("Cannot parse binary");

  for (LIEF::Section sect : bin->sections()) {
    uint64 addr = sect.virtual_address(); // + 0x400000; // for windows
    uint64 size = sect.size();
    auto mem = bin->get_content_from_virtual_address(addr, size);
    printf("[+] Mapping %#08zx-%#08zx\n", addr, addr + size);
    gctx.setConcreteMemoryAreaValue(addr, mem);
  }
  return bin->entrypoint();
}

void emulate(uint64 pc) {
  while (pc) {
    if (!gctx.isConcreteMemoryValueDefined(pc)) {
      printf("Execute dont defined memory %#08zx\n", pc);
      break;
    }
    auto opcodes = gctx.getConcreteMemoryAreaValue(pc, 16);
    auto instr = arch::Instruction(
        pc, reinterpret_cast<uint8 *>(opcodes.data()), opcodes.size());
    if (gctx.processing(instr) != arch::exception_e::NO_FAULT) {
      printf("[-] Invalid instruction: %08X\n", pc);
      break;
    }
    debug("\t\t\t\t|%08zx: %s\n", pc, instr.getDisassembly().data());
    //    if (pc == 0x17b0 || pc == 0x17ed || pc == 0x182a || pc == 0x1863) {
    //    }
    handleHooks(); // jump to hook if pc is RELOC_BASE+offset

    // dont use getNextAddress because it calculate pc as
    // instr_addr+instr_size and handleHooks() set
    // ip-register so instr_addr will be with old value
    pc = getGpr("ip");
  }
}

void initTriton() {
  gctx.setArchitecture(arch::ARCH_X86_64);
  gctx.setMode(modes::ALIGNED_MEMORY, true);
  gctx.setMode(modes::AST_OPTIMIZATIONS, true);
  gctx.setMode(modes::CONSTANT_FOLDING, true);
  gctx.setMode(modes::MEMORY_ARRAY,
               true); // use smt for bitvectors & array(another way only bv)
  gctx.setMode(modes::ONLY_ON_SYMBOLIZED, true);
  // gctx.setMode(modes::PC_TRACKING_SYMBOLIC, true);

  gctx.setAstRepresentationMode(ast::representations::PYTHON_REPRESENTATION);

  gctx.concretizeAllMemory();
  gctx.concretizeAllRegister();

  // setup fake stack regs
  setGpr("sp", 0x9FFFFFFF);
  setGpr("bp", 0x9FFFFFFF);
}

int main(int argc, char *argv[]) {
  initTriton();

  uint64 entrypoint =
      loadExec("/home/l09/Work/CTF/20231220 Knight/krackme/krackme_1.out");
  patchExection(); // bind our _HANDLERS

  puts("[+] Start execution");
  puts("=================================");
  emulate(entrypoint);
  puts("=================================");
  return 0;
}
