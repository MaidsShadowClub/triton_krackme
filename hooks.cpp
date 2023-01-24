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
std::vector<engines::symbolic::SharedSymbolicVariable> syminput;

uint64 __libc_start_main_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);

  auto main_loc = getArg(0);
  std::vector<std::string> argv = {"./programm", "1", "2", "3", "4"};
  uint64 argc = argv.size();

  setStack(2, argc);
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
  //  if (gctx.isMemorySymbolized(str)) {
  //    printf("Strlen symbolized\n");
  //    gctx.createSymbolicExpression()
  //    return 0;
  //  } else {
  uint64 len = lenString(str);
  printf("Strlen: %zd\n", len);
  return len;
  //  }
}

uint64 fgets_HANDLER() {
  debug("[i] Execute %s\n", __FUNCTION__);
  auto buf = getArg(0);
  auto len = getArg(1);
  // gctx.setConcreteMemoryAreaValue(buf, std::string(len, 'A').data(), len);
  gctx.setConcreteMemoryAreaValue(buf, std::string(0x28, 'A').data(), 0x28);

  for (uint64 i = 0; i < 0x28; i++) {
    arch::MemoryAccess mem(buf + i, size::byte);
    std::string comment = "input_" + std::to_string(i);
    syminput.push_back(gctx.symbolizeMemory(mem, comment));
  }
  return 0;
}
