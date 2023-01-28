#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <regex>
#include <triton/archEnums.hpp>
#include <triton/architecture.hpp>
#include <triton/context.hpp>
#include <triton/cpuSize.hpp>
#include <triton/memoryAccess.hpp>
#include <vector>

#include "hooks.hpp"
#include "utils.hpp"

using namespace triton;

extern triton::Context gctx;
extern bool DEBUG;

uint64 __libc_start_main_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto main_loc = getArg(0);
  std::vector<std::string> argv = {"./programm", "1", "2", "3", "4"};
  uint64 argc = argv.size();

  for (int i = 0; i < argc; i++) {
    uint8 *arg = reinterpret_cast<uint8 *>(argv[i].data());
    auto ptr = allocate(arg, argv[i].size() + 1); // +1 for c-string
    setStack(i + 3, ptr);
  }

  // set argc, argv & env
  setArg(0, argc);
  setArg(1, getStack(3));
  setArg(2, 0);

  setGpr("ip", main_loc);
  return 0;
}

// TODO: add format string parser
uint64 printf_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto format = getArg(0);
  uint64 len = lenString(format);
  // va_list ap;
  // va_start(ap, static_cast<uint64>(gctx.getConcreteMemoryValue(format)));
  // printf(readAsciiString(format).data(), ap);
  // va_end(ap);
  printf("%s\n", readUtf8String(format, len).data());
  return 0;
}

uint64 puts_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto format = getArg(0);
  uint64 len = lenString(format);
  printf("%s\n", readUtf8String(format, len).data());
  return 0;
}

uint64 fflush_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);
  return 0;
}

uint64 getlogin_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  unsigned char user[] = "Hacker1337";
  return allocate(user, sizeof(user) + 1); // +1 for c-string
}

uint64 usleep_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto timeout = getArg(0);
  debug("Waiting %zd ms\n", timeout);
  return 0;
}

uint64 sleep_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto timeout = getArg(0);
  debug("Waiting %zd s\n", timeout);
  return 0;
}

uint64 putchar_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto c = getArg(0);
  printf("%c\n", c);
  return c;
}

uint64 exit_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto code = getArg(0);
  printf("Exit: %zd\n", code);
  exit(code);
  return 0;
}

uint64 strlen_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto str = getArg(0);
  uint64 len = lenString(str);
  printf("Strlen: %zd\n", len);
  if (!gctx.isMemorySymbolized(str)) {
    // if syminp[n-2]==0
    // then (ret n-2)
    // else (
    //  if syminp[n-1]==0
    //  then (ret n-1)
    //  else (ret n)
    // )

    printf("Strlen symbolized\n");
    auto actx = gctx.getAstContext();
    engines::symbolic::SharedSymbolicExpression symmem;
    auto ast_if = actx->bv(len, 32); // start creating ast from last element
    for (int i = len - 1; i >= 0; i--) {
      symmem = gctx.getSymbolicMemory(str + i);
      if (symmem == nullptr) {
        printf("[i] Dont symbolized %08x in strlen\n", str + i);
        continue;
      }
      ast_if = actx->ite(actx->equal(actx->reference( // if syminp[i] == 0
                                         symmem),
                                     actx->bv(0, 8)),
                         actx->bv(i, 32), // then return curr len
                         ast_if           // else check next syminps
      );
    }
    auto symexp = gctx.newSymbolicExpression(ast_if);
    auto ret_reg = gctx.getRegister(getGprId("ret"));
    gctx.assignSymbolicExpressionToRegister(symexp, ret_reg);
  }

  return len;
}

uint64 fgets_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto buf = getArg(0);
  auto len = getArg(1);
  // gctx.setConcreteMemoryAreaValue(buf, std::string(len, 'A').data(), len);

  auto actx = gctx.getAstContext();
  ast::SharedAbstractNode ast_char;
  for (int i = 0; i < len; i++) {
    arch::MemoryAccess mem(buf + i, 1);
    auto symmem = gctx.symbolizeMemory(mem);
    ast_char =
        actx->land(actx->bvuge(actx->variable(symmem), actx->bv(0x20, 8)),
                   actx->bvult(actx->variable(symmem), actx->bv(0x7F, 8)));
    auto symexp = gctx.newSymbolicExpression(ast_char);
    printf("node size: %08x; mem size: %08x\n",
           symexp->getAst()->getBitvectorSize(), mem.getBitSize());
    gctx.assignSymbolicExpressionToMemory(symexp, mem);
  }
  return 0;
}
