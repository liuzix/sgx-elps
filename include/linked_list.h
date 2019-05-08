#ifndef INTRUSIVE_LIST_H
#define INTRUSIVE_LIST_H
#include <iostream>
#include <thread>
#include <immintrin.h>
#define MAX_ITER 200
#define POISON_POINTER_DELTA 0
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)
class ListNode
{
public:
    ListNode *prev;
    ListNode *next;
};

template <typename T, typename L, L T::*ln>
class LinkedList {
public:
    L head;
    LinkedList() {
        head.next = &head;
        head.prev = &head;
    }

    T* listEntry(L *nd) {
        if (nd == &head) {
            //std::cout<<"dont' access head"<<std::endl;
            return (T *)0;
        }
        else
            return (T *)((uint64_t)nd- (uint64_t)(&(((T *)0)->ln)));
    }

    int __listAdd(L *newp, L *prev, L *next) {
        //this 
        int cnt = 0, status;
        newp->next = next;
        newp->prev = prev;
        while (cnt < MAX_ITER) {
            if ((status = _xbegin()) == _XBEGIN_STARTED) {
                prev->next = newp;
                next->prev = newp;
                _xend();
                //std::cout<<"inside, adding now"<<std::endl;
                break;
            }
            cnt++;
        }
        return status;
    }

    int listAddHead(L *newp) {
        return __listAdd(newp, &head, head.next);
    }

    int listAddTail(L *newp) {
        return __listAdd(newp, head.prev, &head);
    }

    int listDelete(L *entry) {
        int cnt = 0, status;
        while (cnt < MAX_ITER) {
            if ((status = _xbegin()) == _XBEGIN_STARTED) {
                entry->prev->next = entry->next;
                entry->next->prev = entry->prev;
                _xend();
                //std::cout<<"inside, deleting now"<<std::endl;
                break;
            }
            cnt++;
        }
        entry->next = (L *)LIST_POISON1;
        entry->prev = (L *)LIST_POISON2;
        return status;
    }
};


#endif
