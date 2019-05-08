#ifndef INTRUSIVE_LIST_H
#define INTRUSIVE_LIST_H
#include <iostream>
#include <thread>
#include <immintrin.h>
#include "tsx.h"
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

template <typename T, typename L, typename F, F T::*M>
class LinkedList {
private:
    TransLock listLock;
public:
    L head;
    LinkedList() {
        head.next = &head;
        head.prev = &head;
    }

    L* begin() {
        return head.next;
    }

    L* end() {
        return &head;
    }

    L* next(L* curr) {
        return curr->next;
    }

    L* prev(L* curr) {
        return curr->prev;
    }

    T* listEntry(F *nd) {
        return (T *)((uint64_t)nd- (uint64_t)(&(((T *)0)->*M)));
    }

    void __listAdd(L *newp, L *prev, L *next) {
        newp->next = next;
        newp->prev = prev;
        listLock.lock();
        prev->next = newp;
        next->prev = newp;
        listLock.unlock();
        return;
    }

    void listAddHead(L *newp) {
        return __listAdd(newp, &head, head.next);
    }

    void listAddTail(L *newp) {
        return __listAdd(newp, head.prev, &head);
    }

    bool listDelete(L *entry) {
        listLock.lock();
        if (entry->next == (L *)LIST_POISON1 &&
                entry->prev == (L *)LIST_POISON2) {
            //std::cout<<"deleted twice!" <<std::endl;
            listLock.unlock();
            return false;
        }

        entry->prev->next = entry->next;
        entry->next->prev = entry->prev;
        entry->next = (L *)LIST_POISON1;
        entry->prev = (L *)LIST_POISON2;
        listLock.unlock();
        return true;
    }
};


#endif
