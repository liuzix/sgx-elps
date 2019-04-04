#include "hint.h"

extern "C" int __async_swap(void *addr);
void hint(void *addr) {
    __async_swap(addr);
}