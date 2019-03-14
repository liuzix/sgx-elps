#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <sgx_arch.h>
#include <stdbool.h>
#include <cstddef>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/rbtree.hpp>
using namespace boost::intrusive;

//Define a list that will store MyClass using the public member hook
typedef member_hook< MemoryArea, list_member_hook<>, &MemoryArea::list_hook_> MemberHookList;
typedef list< MemoryArea, MemberHookList> MemberList;

typedef member_hook<MemoryArea, rbtree_member_hook<>,
    &MemoryArea::rbtree_hook_> MemberHookRbtree;
typedef rbtree< MemoryArea, MemberHookRbtree> MemberRbtree;

class MemoryArea {
private:
    size_t len;
    vaddr heapBase;
public:
   //This is a member hook
    list_member_hook<> list_hook_;
    rbtree_member_hook<> rbtree_hook_;  
};

class Allocator {
private:
    size_t len;
    vaddr heapBase;
    MemoryArea root;
    MemoryArea m;
    MemberList chunkList2;
    MemberList chunkList4;
    MemberList chunkList8;
    MemberList chunkList16;
    MemberList chunkList32;
    MemberList chunkList64;
    MemberList chunkList128;
    MemberList chunkList256;
    MemberList chunkList512;
    MemberList chunkList1024;
    MemberList chunkList2048;
public:
    Allocator(size_t len, vaddr heapBase);
    void *la_alloc();
    bool *la_free();
};

#endif
