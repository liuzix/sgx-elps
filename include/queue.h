#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <atomic>
#include "spin_lock.h"


template<typename T>
struct DPointer {
  public:
	union {
		uint64_t ui[2];
		struct {
			T* ptr;
			uint64_t count;
		} __attribute__ (( __aligned__( 16 ) ));
	};

	DPointer() : ptr(nullptr), count(0) {}
	DPointer(T* p) : ptr(p), count(0) {}
	DPointer(T* p, uint64_t c) : ptr(p), count(c) {}

	bool cas(DPointer const& nval, DPointer const& cmp) {
		bool result;
		__asm__ __volatile__ (
			"lock cmpxchg16b %1\n\t"
			"setz %0\n"
			: "=q" ( result )
			,"+m" ( ui )
			: "a" ( cmp.ptr ), "d" ( cmp.count )
			,"b" ( nval.ptr ), "c" ( nval.count )
			: "cc"
		);
		return result;
	}

	// We need == to work properly
	bool operator==(DPointer const&x) {
		return x.ptr == ptr && x.count == count;
	}
};
/*
template<typename T>
class Queue
{
	typedef DPointer<T> Pointer;
	T dummy;
	Pointer Head, Tail;
	std::atomic<int> len;
  public:
	Queue() {
		Head.ptr = Tail.ptr = &this->dummy;
		len = 0;
	}

	int getLen() const {return this->len;}
    void push(T* node) {
		Pointer tail, next;
		do {
			tail = Tail;
			next = tail.ptr->q_next;
			if (tail == Tail) {
				if (next.ptr == nullptr) {
					if (tail.ptr->q_next.cas(Pointer(node, next.count+1), next))
						break;
				} else {
					Tail.cas(Pointer(next.ptr, tail.count+1), tail);
				}
			}
		} while (true);
		Tail.cas(Pointer(node, tail.count+1), tail);
		this->len++;
	}	

	bool take(T*& pvalue) {
		Pointer head, tail, next;
		do {
			head = Head;
			tail = Tail;
			next = head.ptr->q_next;
			if (head == Head) {
				if (head.ptr == tail.ptr) {
					if (next.ptr == nullptr)
						return false;
					Tail.cas(Pointer(next.ptr, tail.count+1), tail);
				} else {
					pvalue = next.ptr;
					if (Head.cas(Pointer(next.ptr, head.count+1), head))
						break;
				}
			}
		} while (true);
		this->len--;
		return true;
	}


};
*/
template<typename T>
class Queue
{
	typedef DPointer<T> Pointer;
	T dummy;
	Pointer Head, Tail;
	SpinLock H_lock, T_lock;
	std::atomic<int> len;
  public:
	Queue() {
		Head.ptr = Tail.ptr = &this->dummy;
	}

	int getLen() const {return this->len;}
	
    void push(T* node) {
		T_lock.lock();
		Tail.ptr->q_next.ptr = node;
		Tail.ptr = node;
		T_lock.unlock();
	}	

	bool take(T*& pvalue) {
		H_lock.lock();
		T* new_head = Head.ptr->q_next.ptr;
		if (new_head == nullptr) {
			H_lock.unlock();
			return false;
		}
		pvalue = new_head;
		Head.ptr = new_head;
		H_lock.unlock();
		return true;
	}



};

#endif
