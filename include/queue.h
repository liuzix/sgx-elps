#ifndef QUEUE_H
#define QUEUE_H

#include <atomic>
#include <cassert>
#include <vector>

#if IS_LIBOS
#include "../libOS/util.h"
#endif

/* Circular lock free queue*/
/*
template<typename T>
class Queue {
  private :
    std::atomic<uint64_t> head;
    std::atomic<uint64_t> r_head;
    std::atomic<uint64_t> tail;
    std::atomic<uint64_t> r_tail;
    std::vector<T> q;
    uint64_t size;
  public :
    std::atomic<int> length;

    Queue() : head(0), r_head(0), tail(0), r_tail(0), q(10), size(10) { }
    Queue(uint64_t s) : head(0), r_head(0), tail(0), r_tail(0), q(s), size(s) {
}

    void push(T v) {
#if IS_LIBOS
    int flag = disableInterrupt();
#endif
        assert(v != nullptr);
        uint64_t pos = tail++;
        while (r_head.load() + size <= pos);
        q[pos % size] = std::move(v);
        __sync_synchronize();
        while (true) {
            uint64_t my_r_tail = r_tail.load();
            if (q[my_r_tail % size] != nullptr && my_r_tail < tail) {
                r_tail.compare_exchange_strong(my_r_tail, my_r_tail + 1);
            } else {
                break;
            }
        }
        length++;
#if IS_LIBOS
        if (!flag)
            enableInterrupt();
#endif
    }

    bool take(T& v) {
        uint64_t pos = head.load();
        if (pos >= r_tail) return false;
        if (!head.compare_exchange_strong(pos, pos + 1)) return false;

        v = std::move(q[pos % size]);
        q[pos % size] = 0x0;
        assert(v != nullptr);
        __sync_synchronize();
        while (true) {
            uint64_t my_r_head = r_head.load();
            if (q[my_r_head % size] == nullptr && my_r_head < head) {
                r_head.compare_exchange_strong(my_r_head, my_r_head + 1);
            } else {
                break;
            }
        }
        length -- ;
        return true;
     }
};
*/

template <typename T> class mpmc_bounded_queue {
public:
    mpmc_bounded_queue(size_t buffer_size = 16)
        : buffer_(new cell_t[buffer_size]), buffer_mask_(buffer_size - 1) {
        assert((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0));
        for (size_t i = 0; i != buffer_size; i += 1)
            buffer_[i].sequence_.store(i, std::memory_order_relaxed);
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
    }

    ~mpmc_bounded_queue() { delete[] buffer_; }

    bool push(T const &data) {
        cell_t *cell;
        size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &buffer_[pos & buffer_mask_];
            size_t seq = cell->sequence_.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)pos;
            if (dif == 0) {
                if (enqueue_pos_.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (dif < 0)
                //return false;
                continue;
            else
                pos = enqueue_pos_.load(std::memory_order_relaxed);
        }
        cell->data_ = data;
        cell->sequence_.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool take(T &data) {
        cell_t *cell;
        size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &buffer_[pos & buffer_mask_];
            size_t seq = cell->sequence_.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
            if (dif == 0) {
                if (dequeue_pos_.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (dif < 0)
                return false;
            else
                pos = dequeue_pos_.load(std::memory_order_relaxed);
        }
        data = cell->data_;
        cell->sequence_.store(pos + buffer_mask_ + 1,
                              std::memory_order_release);
        return true;
    }

private:
    struct cell_t {
        std::atomic<size_t> sequence_;
        T data_;
    };

    static size_t const cacheline_size = 64;
    typedef char cacheline_pad_t[cacheline_size];

    cacheline_pad_t pad0_;
    cell_t *const buffer_;
    size_t const buffer_mask_;
    cacheline_pad_t pad1_;
    std::atomic<size_t> enqueue_pos_;
    cacheline_pad_t pad2_;
    std::atomic<size_t> dequeue_pos_;
    cacheline_pad_t pad3_;

    mpmc_bounded_queue(mpmc_bounded_queue const &);
    void operator=(mpmc_bounded_queue const &);
};

template <typename T>
using Queue = mpmc_bounded_queue<T>;

#endif
