#pragma once
#include <cstring>
#include <cstddef>
#include <cstdint>
#include "panic.h"
#include "util.h"

constexpr size_t n_int64(size_t n_bits) {
    return (n_bits - 1UL) / 64UL + 1UL;
}

template <size_t size>
class Bitmap {
    volatile uint64_t bits[n_int64(size)];
public:
    Bitmap() {
        memset((char *)bits, 0, n_int64(size) << 3);
    }

    __inline__ __attribute__((always_inline))
    int scan_and_set() {
        for (size_t i = 0; i < (size >> 6UL); i++) {
            volatile uint64_t *word;
            word = &bits[i];

            uint64_t cur_word_limit = 64;
            if (((i+1) << 6UL) > size) {
                cur_word_limit = size - (i << 6UL);
            }

            while (1) {
                uint64_t temp = *word;
                uint64_t j = __builtin_ctzl(~temp);
                if (j >= cur_word_limit) goto outer_cont;
                uint64_t temp_new = temp | (1UL << j);
                
                if (__sync_bool_compare_and_swap(word, temp, temp_new))
                    return (i << 6UL) + j ;
            }
outer_cont:
            do {} while (0);
        }
        return -1;
    }

    __inline__ __attribute__((always_inline))
    void unset(size_t i) {
        LIBOS_ASSERT(i < size);

        volatile uint64_t *word = bits + (i >> 6UL);
        size_t slot = i % 64;

        LIBOS_UNUSED uint64_t res;
        res = __sync_fetch_and_and(word, ~(1UL << slot));
        LIBOS_ASSERT(res && (1UL << slot));
    }
} __attribute__((packed));
