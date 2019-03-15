#ifndef QUEUE_ARR_H
#define QUEUE_ARR_H

#include <atomic>
#include <vector>
 
template<typename T>
class Queue {
  private :
	std::atomic<uint64_t> r_head;
    std::atomic<uint64_t> head;
	std::atomic<uint64_t> r_tail;
	std::atomic<uint64_t> tail;
  	std::vector<T> q;
	uint64_t size;
	uint64_t t() {
		return static_cast<uint64_t>(tail);
	}
	uint64_t h() {
		return static_cast<uint64_t>(head);
	}
 
 
   public :
	Queue() : r_head(0),
                        head(0),
                        r_tail(0),
                        tail(0),
						q(10),
						size(10) { }
	Queue(uint64_t s) : r_head(0),
                        head(0),
                        r_tail(0),
                        tail(0),
						q(s),
						size(s) { }
	
	// uses move constructor to avoid locked non-trivial copy
	void push(T v) {
		/* Find a place to write */
		uint64_t pos = r_tail++;
		while (h() + size <=  pos);
		/* do the write */
		q[pos % size] =std:: move(v);
		/* stay you did the write */
		++tail;
	}
	
	void take(T & v) {
		// choose a position to read from
		uint64_t  pos = r_head++;
		// don't read from the tail...
		while (pos >= t());
		v = std::move(q[pos%size]);
		++head;
	}
};

#endif
