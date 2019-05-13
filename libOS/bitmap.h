#pragma once
#include <cstring>
#include <cstddef>
#include <cstdint>
#include "panic.h"
#include "util.h"
#include "tsx.h"

#define BIT_SET_FORCE 1

constexpr size_t n_int64(size_t n_bits) {
    return (n_bits - 1UL) / 64UL + 1UL;
}

template <size_t size, size_t init_value>
class Bitmap {
private:
    volatile uint64_t bits[n_int64(size)];
public:
    TransLock lock;
private:
    /* set [begin, end) bits to 1 if there's no 1 within the range */
    __inline__ __attribute__((always_inline))
    unsigned set_range(uint64_t old_word, size_t begin, size_t end, size_t i) {
        uint64_t word_end = (i == (end >> 6UL)) ? end : ((i + 1) << 6UL);

        unsigned status;
        if ((status = _xbegin()) == _XBEGIN_STARTED) {
            volatile uint64_t *word = &bits[i];
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

                if ((uint64_t)__builtin_ctzl(temp) >= len) {
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
        } else if (_XABORT_CODE(status) == _XABORT_EXPLICIT || _XABORT_CODE(status) == _XABORT_RETRY) {
            libos_print("%s: abort!", __func__);
            return _XABORT_CODE(status);
        }

        return status;
    }

    /* unset at most 1 word bits start from i, see LIBOS_ASSERT for details */
    __inline__ __attribute__((always_inline))
    void unset_word(size_t i, int len) {
        LIBOS_ASSERT(i < size);
        LIBOS_ASSERT(len + (i % 8) <= (sizeof(uint64_t) << 3));
        volatile uint64_t *word = bits;
        uint64_t slot = i % 64;

        word = (uint64_t *)((uint64_t)word + (i >> 3));
        if (len == 64)
            __sync_fetch_and_and(word, 0);
        else
            __sync_fetch_and_and(word, ~((~((~0UL) << len)) << slot) );
    }

public:
    Bitmap() {
        if (init_value)
            memset((char *)bits, 0xff, n_int64(size) << 3);
        else
            memset((char *)bits, 0, n_int64(size) << 3);
    }

    __inline__ __attribute__((always_inline))
    uint64_t getWord(int i) {
        return bits[i];
    }

    void unset_all() {
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

    /*
     * set [start, start + len) bits to 1.
     * When fails, if BIT_SET_FORCE is not given, returns alternative result instead
     */
    __inline__ __attribute__((always_inline))
    int set(size_t start, size_t len, int flag) {
        uint64_t timeout = 0xffffffff;
        do {
            unsigned res = set_range(bits[start >> 6UL], start, start + len, start >> 6UL);
            if ((res == _XABORT_EXPLICIT) && (flag != BIT_SET_FORCE)) {
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
            volatile uint64_t *word = &bits[i];
            if (*word == ~0UL) {
                i++;
                continue;
            }

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
                j = __builtin_ctzl((temp & ((~0UL) << j)));
                if (j == 64) {
                    i++;
                } else {
                    j = __builtin_ctzl((~temp) & (~0UL) << j);
                    goto again;
                }
            } else if (res == _XABORT_RETRY)
                goto retry;
            else
                return (i << 6UL) + j;
        }
        return -1;

    }

    /* return -1 if failed */
    __inline__ __attribute__((always_inline))
    int set(size_t i) {
        LIBOS_ASSERT(i < size);

        volatile uint64_t *word = bits + (i >> 6UL);
        size_t slot = i % 64;

        uint64_t temp, temp_new, timeout = 0xffff;
        bool res;
        do {
            temp = *word;
            temp_new = (temp | 1UL << slot);
            res = __sync_bool_compare_and_swap(word, temp, temp_new);
        } while (!res && timeout--) ;

        if (!res)
            return -1;
        if (temp & (1UL << slot))
            return -1;

        return i;
    }

    /* unset 0 bit is undefined */
    __inline__ __attribute__((always_inline))
    void unset(size_t i) {
        LIBOS_ASSERT(i < size);

        volatile uint64_t *word = bits + (i >> 6UL);
        size_t slot = i % 64;

        LIBOS_UNUSED uint64_t res;
        res = __sync_fetch_and_and(word, ~(1UL << slot));
        LIBOS_ASSERT(res && (1UL << slot));
    }

    /* unset [i, len). unset 0 bit is undefined. */
    __inline__ __attribute__((always_inline))
    void unset(size_t i, int len) {
        LIBOS_ASSERT(i < size);

        if (len == 1) {
            unset(i);
            return ;
        }
        if (len + (i % 8) <= (sizeof(uint64_t) << 3)) {
            /*
             * if i is byte-aligned and len is less than a word
             * or i is not aligned but the bits to be unset can be read as a word
             * then we are good
             */
            unset_word(i, len);
        } else {
            /* or it cannot be lock-free */
            this->lock.lock();

            /* align i to byte */
            uint64_t align_len = (sizeof(uint64_t) << 3) - (i % 8);
            libos_print("unset %lu, len %d", i, align_len);
            unset_word(i, align_len);
            len -= align_len ;
            i += align_len;

            uint64_t word_len = sizeof(uint64_t) << 3;
            while ((unsigned long)len > word_len) {
                libos_print("unset %lu, len %d", i, word_len);
                unset_word(i, word_len);
                len -= word_len;
                i += word_len;
            }

            libos_print("unset %lu, len %d", i, len);

            unset_word(i, len);
            this->lock.unlock();
        }

        return ;
    }

} __attribute__((packed));
