//! \file
/*
 **  This program is under the terms of the Apache License 2.0.
 **  Jonathan Salwan
 */

#include <cstdarg>
#include <cstddef>
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

#include <cstdio>

#include "routines.hpp"
#include "ttexplore.hpp"
#include "utils.hpp"

extern bool DEBUG;

/*
 * This file aims to provide an example about using routines when emulating a
 * target. For example, we provide a very simple printf routine that just prints
 * the string format pointed by rdi. As example, this printf routine is used in
 * the harness5.
 *
 * The idea behind routines is that you can simulate whatever the program calls
 * and update the triton context according to your goals.
 */

namespace triton {
namespace routines {
triton::callbacks::cb_state_e __libc_start_main(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

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
  return triton::callbacks::CONTINUE;
}

// TODO: add format string parser
triton::callbacks::cb_state_e printf(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

  auto format = getArg(0);
  uint64 len = lenString(format);
  // va_list ap;
  // va_start(ap, static_cast<uint64>(gctx.getConcreteMemoryValue(format)));
  // printf(readAsciiString(format).data(), ap);
  // va_end(ap);
  std::printf("%s\n", readUtf8String(format, len).data());
  return triton::callbacks::PLT_CONTINUE;
}

triton::callbacks::cb_state_e puts(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

  auto format = getArg(0);
  uint64 len = lenString(format);
  std::printf("%s\n", readUtf8String(format, len).data());
  return triton::callbacks::PLT_CONTINUE;
}

triton::callbacks::cb_state_e fflush(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);
  return triton::callbacks::PLT_CONTINUE;
}

triton::callbacks::cb_state_e getlogin(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

  unsigned char user[] = "Hacker1337";
  setGpr("ret", allocate(user, sizeof(user) + 1)); // +1 for c-string
  return triton::callbacks::PLT_CONTINUE;
}

triton::callbacks::cb_state_e usleep(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

  auto timeout = getArg(0);
  debug_printf("Waiting %zd ms\n", timeout);
  return triton::callbacks::PLT_CONTINUE;
  callbacks::PLT_CONTINUE;
}

triton::callbacks::cb_state_e sleep(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

  auto timeout = getArg(0);
  debug_printf("Waiting %zd s\n", timeout);
  return triton::callbacks::PLT_CONTINUE;
}

triton::callbacks::cb_state_e putchar(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

  auto c = static_cast<uint8>(getArg(0));
  std::printf("%c", c);
  debug_puts("\n");
  setGpr("ret", c);
  return triton::callbacks::PLT_CONTINUE;
}

triton::callbacks::cb_state_e exit(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

  auto code = getArg(0);
  triton_printf("Exit: %zd\n", code);
  // std::exit(code);
  return triton::callbacks::BREAK;
}

triton::callbacks::cb_state_e fgets(triton::Context *ctx) {
  debug_printf("[i] Execute %s\n", __FUNCTION__);

  auto buf = getArg(0);
  auto len = getArg(1);
  if (ctx->isMemorySymbolized(buf)) {
    return triton::callbacks::PLT_CONTINUE;
  }

  triton_printf("[!] fgets: add ctx->symbolizeMemory(%#010zx, %#zx)", buf, len);
  return triton::callbacks::PLT_CONTINUE;
}

triton::callbacks::cb_state_e stub(triton::Context *ctx) {
  return triton::callbacks::CONTINUE;
}
}; // namespace routines
}; // namespace triton
