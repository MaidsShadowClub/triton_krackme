#include <LIEF/ELF.hpp>
#include <exception>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <triton/archEnums.hpp>
#include <triton/ast.hpp>
#include <triton/callbacks.hpp>
#include <triton/callbacksEnums.hpp>
#include <triton/context.hpp>
#include <triton/cpuSize.hpp>
#include <triton/exceptions.hpp>
#include <triton/instruction.hpp>
#include <triton/memoryAccess.hpp>
#include <triton/modesEnums.hpp>
#include <triton/register.hpp>
#include <triton/stubs.hpp>

#include "routines.hpp"
#include "ttexplore.hpp"
#include "utils.hpp"

using namespace triton;

triton::Context gctx;
bool DEBUG = false;
std::unique_ptr<const LIEF::ELF::Binary> bin;

#define RELOC_BASE 0x10000000
#define STUB_BASE 0x11000000
#define STACK_BASE 0x9FFFFFFF

#define HANDLER(name)                                                          \
  {                                                                            \
#name, {                                                                   \
      ROUTINE, RELOC_BASE + __COUNTER__ *size::dword, triton::routines::name   \
    }                                                                          \
  }
// __COUNTER__ used to determine hook-function in callback
#define STUB_HANDLER(name)                                                     \
  {                                                                            \
    #name, {                                                                   \
      LIBC_STUB,                                                               \
          triton::stubs::x8664::systemv::libc::symbols.at(#name) + STUB_BASE,  \
          triton::routines::stub                                               \
    }                                                                          \
  }

enum plt_type { ROUTINE = 0, LIBC_STUB, LIBC_CUSTOM };

struct plt_info {
  triton::uint8 type;
  triton::uint64 addr;
  triton::engines::exploration::instCallback cb;
};

// add relocation or symbol you would like to hook
std::map<std::string, plt_info> custom_plt{HANDLER(__libc_start_main),
                                           HANDLER(printf),
                                           HANDLER(puts),
                                           HANDLER(fflush),
                                           HANDLER(getlogin),
                                           HANDLER(usleep),
                                           HANDLER(putchar),
                                           HANDLER(exit),
                                           STUB_HANDLER(strlen),
                                           HANDLER(fgets)};

void patchExection() {
  for (const LIEF::Relocation rel : bin->relocations()) {
    auto addr = rel.address();
    auto symb = bin->get_relocation(addr)->symbol();
    if (symb == nullptr)
      continue;

    auto name = symb->name();
    auto check_hook = [=](std::pair<std::string, plt_info> i) {
      return i.first == name;
    };
    auto hook = std::find_if(custom_plt.begin(), custom_plt.end(), check_hook);
    if (hook == custom_plt.end())
      continue;

    triton_printf("[i] Replacing reloc %s\n", name.data());
    arch::MemoryAccess mem(addr, gctx.getGprSize());
    gctx.setConcreteMemoryValue(mem, hook->second.addr);
  }
  for (const LIEF::Symbol symb : bin->symbols()) {
    auto addr = symb.value();
    if (addr == 0)
      continue;

    auto name = symb.name();
    auto check_hook = [=](std::pair<std::string, plt_info> i) {
      return i.first == name;
    };
    auto hook = std::find_if(custom_plt.begin(), custom_plt.end(), check_hook);
    if (hook == custom_plt.end())
      continue;

    triton_printf("[i] Replacing symb %s\n", name.data());
    arch::MemoryAccess mem(addr, gctx.getGprSize());
    gctx.setConcreteMemoryValue(mem, hook->second.addr);
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
    triton_printf("[+] Mapping %#08zx-%#08zx\n", addr, addr + size);
    gctx.setConcreteMemoryAreaValue(addr, mem);
  }

  gctx.setConcreteMemoryAreaValue(STUB_BASE,
                                  triton::stubs::x8664::systemv::libc::code);
  return bin->entrypoint();
}

void initTriton() {
  gctx.setArchitecture(arch::ARCH_X86_64);
  gctx.setMode(modes::ALIGNED_MEMORY, true);
  gctx.setMode(modes::AST_OPTIMIZATIONS, true);
  gctx.setMode(modes::CONSTANT_FOLDING, true);
  // gctx.setMode(modes::MEMORY_ARRAY,
  //              true); // use smt for bitvectors & array(another way only bv)
     gctx.setMode(modes::ONLY_ON_SYMBOLIZED, true);
  // gctx.setMode(modes::PC_TRACKING_SYMBOLIC, true);

  gctx.setAstRepresentationMode(ast::representations::SMT_REPRESENTATION);

  // setup fake stack regs
  setGpr("sp", STACK_BASE);
  setGpr("bp", STACK_BASE);
}

// add symbolic for memory
void symbolize() {
  gctx.symbolizeMemory(0x9fffff40, 0x32);
}

int main(int argc, char *argv[]) {
  initTriton();

  uint64 entrypoint =
      loadExec("/home/l09/Work/CTF/20231220 Knight/krackme/krackme_1.out");
  patchExection(); // bind our hooks

  auto reg = gctx.getRegister(getGprId("ip"));
  gctx.setConcreteRegisterValue(reg, entrypoint);
  symbolize();

  /* Setup exploration */
  engines::exploration::SymbolicExplorator explorator;

  for (auto plt : custom_plt) {
    if (plt.second.type == ROUTINE)
      explorator.hookInstruction(plt.second.addr, plt.second.cb);
  }

  explorator.initContext(&gctx); /* define an initial context */
  explorator.explore();          /* do the exploration */
  return 0;
}
