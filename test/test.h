#include <atomic>

#ifndef TEST_H_
#define TEST_H_

/*https://blog.lse.epita.fr/articles/42-implementing-generic-double-word-compare-and-swap-.html*/

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

template<typename T>
class Queue
{
	struct Node;
	typedef DPointer<Node> Pointer;

	struct Node {
		T value;
		Pointer next;

		Node() : next(nullptr) {}
		Node(T x, Node* nxt) : value(x), next(nxt) {}
	};

	Pointer Head, Tail;
	atomic<int> len;
  public:
	Queue() {
		Node *node = new Node();
		Head.ptr = Tail.ptr = node;
	}

	int getLen() const {return this->len;}
	void push(T x) {
		Node *node = new Node(x, nullptr);
		Pointer tail, next;
		do {
			tail = Tail;
			next = tail.ptr->next;
			if (tail == Tail) {
				if (next.ptr == nullptr) {
					if (tail.ptr->next.cas(Pointer(node, next.count+1), next))
						break;
				} else {
					Tail.cas(Pointer(next.ptr, tail.count+1), tail);
				}
			}
		} while (true);
		Tail.cas(Pointer(node, tail.count+1), tail);
		this->len++;
	}

	bool take(T& pvalue) {
		Pointer head, tail, next;
		do {
			head = Head;
			tail = Tail;
			next = head.ptr->next;
			if (head == Head) {
				if (head.ptr == tail.ptr) {
					if (next.ptr == nullptr)
						return false;
					Tail.cas(Pointer(next.ptr, tail.count+1), tail);
				} else {
					pvalue = next.ptr->value;
					if (Head.cas(Pointer(next.ptr, head.count+1), head))
						break;
				}
			}
		} while (true);
		delete(head.ptr);
		this->len--;
		return true;
	}
};



#endif /* BASIC_H_ */

