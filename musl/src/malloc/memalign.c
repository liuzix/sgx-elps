#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>
#include "malloc_impl.h"

void *__memalign(size_t align, size_t len)
{
    return mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

weak_alias(__memalign, memalign);
