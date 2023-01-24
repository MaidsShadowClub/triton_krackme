#include <triton/context.hpp>

using namespace triton;

uint64 getStack(int number);
void setStack(int number, const uint64 value);

uint64 __libc_start_main_HANDLER();
uint64 printf_HANDLER();
uint64 puts_HANDLER();
uint64 fflush_HANDLER();
uint64 getlogin_HANDLER();
uint64 usleep_HANDLER();
uint64 sleep_HANDLER();
uint64 putchar_HANDLER();
uint64 exit_HANDLER();
uint64 strlen_HANDLER();
uint64 fgets_HANDLER();
