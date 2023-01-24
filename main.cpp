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
#define STACK_BASE 0x9FFFFFFF
#define NULL_HANDLER                                                           \
  {                                                                            \
    0, { "stub", NULL }                                                        \
  }
#define HANDLER(name)                                                          \
  {                                                                            \
    RELOC_BASE + __COUNTER__ *size::dword, {                                   \
#name, (uint64(*)()) & name##_HANDLER                                    \
    }                                                                          \
  }
// __COUNTER__ used to determine hook-function in callback

struct hook_info {
  std::string name;
  uint64 (*func)();
};

// add relocation you would like to hook
std::map<uint64, hook_info> custom_rels{HANDLER(__libc_start_main),
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

void relocCb(Context &ctx, const arch::Register &reg) {
  if (reg.getId() != getGprId("ip"))
    return;
  auto pc = getGpr("ip");
  if (custom_rels.count(pc) == 0)
    return;

  auto rel = custom_rels[pc];
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

void setRelocCb() {
  // make RELOC's addresses defined in gctx memory
  gctx.setConcreteMemoryAreaValue(
      RELOC_BASE, std::vector<uint8>(custom_rels.size() * size::dword, 0));

  // execute our hook when ip == RELOC_BASE+%HANDLER_OFFSET%
  gctx.addCallback(callbacks::GET_CONCRETE_REGISTER_VALUE, relocCb);
}

void patchRelocs() {
  auto rels = bin->relocations();
  for (const auto rel : rels) {
    auto addr = rel.address();
    auto symb = bin->get_relocation(addr)->symbol();
    if (symb == nullptr)
      continue;

    std::pair<uint64, hook_info> hook = NULL_HANDLER;
    auto name = symb->name();
    for (auto hndl : custom_rels) {
      if (hndl.second.name == name) {
        hook = hndl;
        break;
      }
    }
    if (hook.first == 0)
      continue;

    printf("[i] Hooking %s\n", name.data());
    arch::MemoryAccess mem(addr, gctx.getGprSize());
    gctx.setConcreteMemoryValue(mem, hook.first);
  }
}

uint64_t loadExec(std::string str) {
  bin = LIEF::ELF::Parser::parse(str);
  if (bin == nullptr)
    throw std::invalid_argument("Cannot parse binary");

  for (const LIEF::ELF::Segment &segm : bin->segments()) {
    uint64 addr = segm.virtual_address();
    uint64 size = segm.physical_size();
    auto mem = segm.content().data();
    printf("[+] Mapping %#08zx-%#08zx\n", addr, addr + size);
    gctx.setConcreteMemoryAreaValue(addr, mem, size);
  }
  return bin->entrypoint();
}

void emulate(uint64 pc) {
  setRelocCb(); // set callback to execute _HANDLER funcs
  while (pc) {
    if (!gctx.isConcreteMemoryValueDefined(pc)) {
      printf("Execute dont defined memory %#08zx\n", pc);
      break;
    }
    auto opcodes = gctx.getConcreteMemoryAreaValue(pc, 16);
    auto instr = arch::Instruction(
        pc, reinterpret_cast<uint8 *>(opcodes.data()), opcodes.size());
    gctx.processing(instr);
    if (instr.getDisassembly() == "hlt")
      break;
    debug("\t\t\t\t|%08zx: %s\n", pc, instr.getDisassembly().data());
    //    if (pc == 0x17b0 || pc == 0x17ed || pc == 0x182a ||
    //        pc == 0x1863) { // 0x18c9) {
    //      ast::SharedAbstractNode pred = gctx.getPathPredicate();
    //      ast::SharedAstContext ast = gctx.getAstContext();

    //      // std::cout << "In: " << ast::unroll(pred) << std::endl;
    //      ast::SharedAbstractNode simp = gctx.simplifyAstViaSolver(pred);
    //      std::cout << "Ip: " << pc << "; out: " << simp << std::endl;
    //      // printf("%08x\n",
    //      instr.operands[0].getConstMemory().getAddress());
    //      // ast::SharedAbstractNode node =
    //      gctx.getOperandAst(instr.operands[0]);
    //    }
    pc = getGpr("ip");
  }
  auto simps = gctx.getPredicatesToReachAddress(0x18d2);
  printf("Total: %d predicates\n", simps.size());
  for (auto simp : simps) {
    std::cout << "Solver: " << gctx.simplifyAstViaSolver(simp);
    std::cout << "; llvm: " << gctx.simplifyAstViaLLVM(simp) << std::endl;
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
  gctx.setMode(modes::PC_TRACKING_SYMBOLIC, true);

  gctx.setAstRepresentationMode(ast::representations::PYTHON_REPRESENTATION);

  gctx.concretizeAllMemory();
  gctx.concretizeAllRegister();

  // setup stack regs
  setGpr("sp", STACK_BASE);
  setGpr("bp", STACK_BASE);
}

int main(int argc, char *argv[]) {
  initTriton();

  uint64 entrypoint =
      loadExec("./krackme_1.out");
  patchRelocs(); // init our reloc's implementation

  puts("[+] Start execution");
  puts("=================================");
  emulate(entrypoint);
  puts("=================================");
  return 0;
}
