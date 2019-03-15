#ifndef QUEUE_H
#define QUEUE_H

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
   /*
        while (true) {
            uint64_t curr_tail = tail.load();
            uint64_t next_tail = curr_tail + 1;
            if (next_tail > r_head.load()) {
                if (tail.compare_exchange_weak(curr_tail, next_tail)) {
                    q[curr_tail % size] = std::move(v);
                    while (q[r_tail.load() % size] == 0x0);
                    r_tail++;
                    break;
                }
            }
        }
*/
        uint64_t pos = tail++;
        while (r_head.load() + size <= pos);
        q[pos % size] = std::move(v);
        while (q[r_tail.load() % size] == 0x0); 
        ++r_tail;
    }

    void take(T& v) {
/*
        while (true) {
            uint64_t curr_head = head.load();
            if (curr_head < r_tail.load()) {
                if (head.compare_exchange_weak(curr_head, curr_head+1)) {
                    v = q[curr_head % size];
                    q[curr_head % size] = 0x0;
                    while (q[r_head.load() % size] != 0x0);
                    r_head++;
                    break;
                }
            }
        }
*/   
        uint64_t pos = head++;
        while (pos >= r_tail.load());
        v = std::move(q[pos % size]);
        q[pos % size] = 0x0;
        while (q[r_head.load() % size] != 0x0);
        ++r_head;
     }
};

#endif
