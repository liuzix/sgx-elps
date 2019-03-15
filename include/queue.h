#ifndef QUEUE_H
#define QUEUE_H

#include <cassert>
#include <atomic>
#include <vector>
 
/* Circular lock free queue*/
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
    Queue() : head(0), r_head(0), tail(0), r_tail(0), q(10), size(10) { }
    Queue(uint64_t s) : head(0), r_head(0), tail(0), r_tail(0), q(s), size(s) { }

    void push(T v) {
        uint64_t pos = tail++;
        while (r_head.load() + size <= pos);
        q[pos % size] = std::move(v);

        while (true) {
            uint64_t my_r_tail = r_tail.load();
            if (q[my_r_tail % size] != nullptr && my_r_tail < tail) {
                r_tail.compare_exchange_weak(my_r_tail, my_r_tail + 1);
            } else {
                break;
            }
        }
    }

    bool take(T& v) {
        uint64_t pos = head.load();
        if (pos == r_tail) return false;
        if (!head.compare_exchange_weak(pos, pos + 1)) return false;
        
        v = std::move(q[pos % size]);
        q[pos % size] = 0x0;
        
        while (true) {
            uint64_t my_r_head = r_head.load();
            if (q[my_r_head % size] == nullptr && my_r_head < head) {
                r_head.compare_exchange_weak(my_r_head, my_r_head + 1);
            } else {
                break;
            }
        }

        return true;
     }
};

#endif
