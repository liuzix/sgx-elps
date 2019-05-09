#include <iostream>
#include <thread>
#include <immintrin.h>
#include "../libOS/linked_list.h"
#include <algorithm>    // std::for_each
#include <vector>       // std::vector
/*
#define container_of(ptr, type, member) ({                     \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)
#define MAX_ITER 200
#define POISON_POINTER_DELTA 0
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)
struct list_head {
    struct list_head *next, *prev;
};
int glb = 0;
int initListHead(struct list_head *list) {
    int cnt = 0, status;
    while (cnt < MAX_ITER) {
        if ((status = _xbegin()) == _XBEGIN_STARTED) {
            list->next = list;
            list->prev = list;
            _xend();
            std::cout<<"inside";
            break;
        }
        cnt++;
    }
    return status;
}
int __listAdd(struct list_head *newp, struct list_head *prev, struct list_head *next) {
    int cnt = 0, status;
    newp->next = next;
    newp->prev = prev;
    while (cnt < MAX_ITER) {
        if ((status = _xbegin()) == _XBEGIN_STARTED) {
            prev->next = newp;
            next->prev = newp;
            _xend();
            std::cout<<"inside, adding now"<<std::endl;
            break;
        }
        cnt++;
    }
    return status;
}
int listAddHead(struct list_head *newp, struct list_head *head) {
    return __listAdd(newp, head, head->next);
}

int listAddTail(struct list_head *newp, struct list_head *head) {
    return __listAdd(newp, head->prev, head);
}

int listDelete(struct list_head *entry) {
    int cnt = 0, status;
    while (cnt < MAX_ITER) {
        if ((status = _xbegin()) == _XBEGIN_STARTED) {
            entry->prev->next = entry->next;
            entry->next->prev = entry->prev;
            _xend();
            std::cout<<"inside, deleting now"<<std::endl;
            break;
        }
        cnt++;
    }
    entry->next = (list_head *)LIST_POISON1;
    entry->prev = (list_head *)LIST_POISON2;
    return status;
}
*/


struct fox {
    unsigned long tail_length; /* length in centimeters of tail */
    unsigned long weight; /* weight in kilograms */
    int test;
    bool is_fantastic; /* is this fox fantastic? */
    ListNode ln; /* list of all fox structures */
};
/*
void do_join(std::thread& t)
{
    t.join();
}

void join_all(std::vector<std::thread>& v)
{
    std::for_each(v.begin(),v.end(),do_join);
}
*/
int main (void) {
    struct fox f1[5], f2[5], f3[5];
    for (int i = 0; i < 5; i++) {
        f1[i].test = i * 10 + 1;
        f2[i].test = i * 10 + 4;
        f3[i].test = i * 10 + 8;
        std::cout<<"it should be: "<<f1[i].test<<" "<<f2[i].test<<" " << f3[i].test<<std::endl;
    }

    LinkedList<fox, ListNode, ListNode, &fox::ln> lst;
    for (int i =0; i < 5; i++) {
        lst.listAddHead(&f1[i].ln);
        lst.listAddHead(&f2[i].ln);
        lst.listAddHead(&f3[i].ln);
    }
    /*
    std::vector<std::thread> v;
    for (int i = 0; i < 5; i++) {
        v.push_back(std::thread(&LinkedList<fox, ListNode, ListNode, &fox::ln>::listAddHead, &lst, &f1[i].ln));
        v.push_back(std::thread(&LinkedList<fox, ListNode,ListNode,  &fox::ln>::listAddHead, &lst, &f2[i].ln));
        v.push_back(std::thread(&LinkedList<fox, ListNode,ListNode,  &fox::ln>::listAddHead, &lst, &f3[i].ln));
    }
     join_all(v);
     */
     auto beg = lst.end();
     beg = lst.prev(beg);
     while (beg != lst.end()) {
        auto tp1 = lst.listEntry(beg);
        std::cout<<"this is:  "<<tp1->test<<std::endl;
        beg = lst.prev(beg);
     }
     if (lst.listDelete(&f3[1].ln))
         std::cout<<"successfully deleted"<<std::endl;
    
     if (lst.listDelete(&f3[1].ln))
         std::cout<<"successfully deleted"<<std::endl;
     else
         std::cout<<"failed to delete"<<std::endl;
/*

    ListNode *tmp;
    tmp = &lst.head;
    for(int i = 0; i < 5 ; i++) {
        std::cout<<lst.listEntry(&f1[i].is_fantastic)->test<<std::endl;
        std::cout<<lst.listEntry(&f2[i].is_fantastic)->test<<std::endl;
        std::cout<<lst.listEntry(&f3[i].is_fantastic)->test<<std::endl;
    }
    lst.listDelete(&f1[2].ln);
    std::cout<<"========================================="<<std::endl;
    for(int i = 0; i < 5 ; i++) {
        std::cout<<lst.listEntry(&f1[i].is_fantastic)->test<<std::endl;
        std::cout<<lst.listEntry(&f2[i].is_fantastic)->test<<std::endl;
        std::cout<<lst.listEntry(&f3[i].is_fantastic)->test<<std::endl;
    }
    initListHead(&f1.list);
    std::cout<<listAddHead(&f2.list, &f1.list);
    std::cout<<listAddTail(&f3.list, &f1.list);
    struct fox *f11, *f12;
    std::cout<< "=================================="<<std::endl;
    f11 = list_entry(f1.list.next, struct fox, list);
    f12 = list_entry(f1.list.prev, struct fox, list);
    std::cout<<f11->weight<<std::endl;
    std::cout<<f12->weight<<std::endl;
    listDelete(&f2.list);

    f11 = list_entry(f1.list.next, struct fox, list);
    f12 = list_entry(f1.list.prev, struct fox, list);

    std::cout<<f11->weight<<std::endl;
    std::cout<<f12->weight<<std::endl;
   */
    return 0;
}
