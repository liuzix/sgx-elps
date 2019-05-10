#ifndef HINT_H
#define HINT_H
#include <immintrin.h>
void hint(void *addr);
typedef unsigned long uint64_t;
extern uint64_t *pjiffies;

static inline void cond_hint(void *addr) {
    
}

#endif
