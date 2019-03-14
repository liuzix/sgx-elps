#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <sgx_arch.h>
#include <stdbool.h>
#include <cstddef>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/rbtree.hpp>
#define CHUNK_LIST_SIZE 20
using namespace boost::intrusive;

//Define a list that will store MyClass using the public member hook
class MemoryArea {
private:
public:
    MemoryArea(size_t l);
    size_t len;
   //This is a member hook
    list_member_hook<> list_hook_;
    set_member_hook<> rbtree_hook_;
};

typedef member_hook<MemoryArea, list_member_hook<>, &MemoryArea::list_hook_> MemberHookList;
typedef list<MemoryArea, MemberHookList> MemberList;

typedef member_hook<MemoryArea, set_member_hook<>,
    &MemoryArea::rbtree_hook_> MemberHookRbtree;
typedef rbtree<MemoryArea, MemberHookRbtree> MemberRbtree;

class Allocator {
private:
    vaddr heapStart;
    vaddr heapEnd;
    MemberRbtree root;
    MemberList chunkList[CHUNK_LIST_SIZE];
public:
    size_t len;
    Allocator(size_t len, vaddr heapBase);
    void *malloc(size_t len);
    void free(vaddr baseAddr);
};


#endif
