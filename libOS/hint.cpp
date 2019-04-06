#include "panic.h"
#include "hint.h"

extern "C" int __async_swap(void *addr);
void hint(void *addr) {
    //uint64_t jiffies = *pjiffies;
    __async_swap(addr);
    //libos_print("jiffies: %ld", *pjiffies - jiffies);
}
