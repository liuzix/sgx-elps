#pragma once
#include <cstring>
#include <cstddef>
#include <cstdint>
#include "panic.h"
#include "util.h"

#define BIT_SET_FORCE 1;

constexpr size_t n_int64(size_t n_bits) {
    return (n_bits - 1UL) / 64UL + 1UL;
}

template <size_t size, size_t init_value>
class Bitmap {
private:
    volatile uint64_t bits[n_int64(size)];

    /* set [begin, end) bits to 1 if there's no 1 within the range */
    __inline__ __attribute__((always_inline))
    unsigned set_range(size_t begin, size_t end, size_t i) {
        uint64_t word_end = (i == (end >> 6UL)) ? end : ((i + 1) << 6UL);

        unsigned status;
        if ((status = _xbegin()) == _XBEGIN_STARTED) {
            uint64_t *word = &bit[i];
            if (*word != old_word)
                _xabort(_XABORT_RETRY);
            while ((int)word_end - 64 < (int)end) {
                uint64_t j = begin % 64;
                uint64_t k = word_end < end ? word_end % 64 : end % 64;
                uint64_t len;
                uint64_t temp;

                if (!k) {
                    len = 64 - j;
                    temp = ((*word) >> j);
                } else {
                    len = k - j;
                    temp = ((*word) >> j) | (1UL << len);
                }

                if (__builtin_ctzl(temp) >= len) {
                    if (len == 64)
                        temp = ~0UL;
                    else
                        temp = (~((~0UL) << len)) << begin;
                    *word = (*word) | temp;
                } else {
                    _xabort(_XABORT_EXPLICIT);
                }
                i++;
                begin = word_end;
                word_end += 64;
            }
            _xend();
        } else if (_XABORT_CODE(status) == _XABORT_EXPLICIT || _XABORT_CODE(status) == _XABORT_RETRY)
            return _XABORT_CODE(status);

        return status;
    }

    /* unset at most 1 word bits start from i*/
    __inline__ __attribute__((always_inline))
    void unset_word(size_t i, int len) {
        LIBOS_ASSERT(i < size);
        LIBOS_ASSERT(len < sizeof(uint64_t));
        volatile uint64_t *word = bits + (i >> 6UL);
        size_t slot = i % 64;

        word = (uint64_t *)((uint64_t)word + i);
        res = __sync_fetch_and_and(word, ~((~0UL) << len));
    }

public:
    Bitmap() {
        memset((char *)bits, init_value, n_int64(size) << 3);
    }

    void unset_all() {
        memset((char *)bits, 1, n_int64(size) << 3);
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

            uint64_t timeout = 0xffffffff;
            while (timeout--) {
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

    /* set [start, start + len) bits to 1 */
    __inline__ __attribute__((always_inline))
    int set(size_t start, size_t len, int flag) {
        uint64_t timeout = 0xffffffff;
        do {
            unsigned res = set_range(start, start + len, start >> 6UL);
            if (res == _XABORT_EXPLICIT && flag != BIT_SET_FORCE) {
                return scan_and_set(len);
            } else if (res == _XABORT_EXPLICIT)
                return -1;
        } while (timeout--);
        return -1;
    }

    __inline__ __attribute__((always_inline))
    int scan_and_set(size_t len) {
        for (size_t i = 0; i < (size >> 6UL);) {
retry:
            volatile uint64_t *word = &bit[i];
            if (*word == ~0UL) {
                i++;
                continue;
            }

            uint64_t cur_word_limit = 64;
            if (((i+1) << 6UL) > size)
                cur_word_limit = size - (i << 6UL);

            uint64_t temp = *word;
            uint64_t j = __builtin_ctzl(~temp);

again:
            uint64_t timeout = 0xffffffff;
            unsigned res = 0;
            size_t begin = (i << 6UL) + j;

            if (begin > size || begin + len > size)
                return -1;
            res = set_range(*word, begin, begin + len, i);
            if (!timeout)
                return -1;
            timeout--;
            if (res == _XABORT_EXPLICIT) {
                size_t old = j;
                j = __builtin_ctzl((temp & ((~0UL) << j)));
                if (j == 64) {
                    cout << "next" << endl;
                    i++;
                } else {
                    j = __builtin_ctzl((~temp) & (~0UL) << j);
                    goto again;
                }
                cout << "i: " << i << endl;
            } else if (res == _XABORT_RETRY)
                goto retry;
            else
                return (i << 6UL) + j;
        }
        return -1;

    }

    __inline__ __attribute__((always_inline))
    int set(size_t i) {
        return set(i, 1, BIT_SET_FORCE);
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

    __inline__ __attribute__((always_inline))
    void unset(size_t i, int len) {
        LIBOS_ASSERT(i < size);

        while (len > sizeof(uint64_t)) {
            unset_word(i, sizeof(uint64_t));
            len -= sizeof(uint64_t);
            i += sizeof(uint64_t);
        }

        unset_word(i, len);
        return ;
    }

    __inline__ __attribute__((always_inline))
    bool get_and_set(size_t index) {
        return (set(index) != -1);
    }
} __attribute__((packed));
