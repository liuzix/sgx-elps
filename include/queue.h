#ifndef QUEUE_ARR_H
#define QUEUE_ARR_H

#include <atomic>
#include <vector>
 
template<typename T>
class Queue {
  private :
    std::atomic<uint64_t> head;
	std::atomic<uint64_t> tail;
  	std::vector<T> q;
	uint64_t size;
 	uint64_t increment(uint64_t n) {
		return (n + 1) % size;
	}
 
   public :
	Queue() :           head(0),
                        tail(0),
						q(10),
						size(10) { }
	Queue(uint64_t s) : head(0),
                        tail(0),
						q(s),
						size(s) { }
	
	bool try_push(T& v) {
		uint64_t curr_tail = tail.load();
		uint64_t next_tail = increment(curr_tail);
		if (next_tail != head.load()) {
			q[curr_tail] = std::move(v);
			tail.store(next_tail);
			return true;
		}
		return false;
	}
	void push(T& v) {
		while (!try_push(v));
	}
	
	bool try_take(T& v) {
		uint64_t curr_head = head.load();
		if (curr_head == tail.load())
			return false;
		v = q[curr_head];
		head.store(increment(curr_head));
		return true;
	}
	void take(T & v) {
		while (!try_take(v));
	}
};

#endif
