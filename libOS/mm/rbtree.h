#pragma once
#include <libOS/panic.h>
#include <libOS/tsx.h>
#include <spin_lock.h>
#include <cstdint>


#define LOCK_TYPE TransLock
namespace mm {

template <typename T,
          typename R,
          R T::*M
         >
constexpr std::size_t offset_of()
{
    return reinterpret_cast<std::size_t>(&(((T*)0)->*M));
};

enum Color {RED, BLACK};

struct malloc_rbtree_base;
struct rbtree_node {
    Color color;
    rbtree_node *left, *right, *parent;

    void set_left(rbtree_node *node) {
        this->left = node;
        if (node)
            node->parent = this;
    }

    void set_right(rbtree_node *node) {
        this->right = node;
        if (node)
            node->parent = this;
    }
};

struct malloc_rbtree_key_base {
private:
    virtual void dummy() = 0;
};

struct malloc_rbtree_base {
private:

    bool insert_bst(rbtree_node *node);
    virtual bool less_than(rbtree_node *n1, rbtree_node *n2) = 0;
    virtual bool less_than(malloc_rbtree_key_base *key, rbtree_node *node) = 0;
    virtual bool less_than(rbtree_node *node, malloc_rbtree_key_base *key) = 0;

    void rotation_left(rbtree_node *p); 
    void rotation_right(rbtree_node *p);

    /* for deletion */
    void transplant(rbtree_node *u, rbtree_node *v);
    void delete_fixup(rbtree_node *x);
   
    /* for debugging */
    void verify_tree(rbtree_node *node);
protected:
    LOCK_TYPE lock;
    rbtree_node *root = nullptr;
    rbtree_node sentinel;

    malloc_rbtree_base() {
        sentinel.color = BLACK;
    }
    
    bool not_null(rbtree_node *p) {
        return p != &sentinel && p != nullptr;
    }

    bool __insert(rbtree_node *node);
    rbtree_node *__lookup(malloc_rbtree_key_base *key);
    rbtree_node *__erase(rbtree_node *node);
    rbtree_node *__next(rbtree_node *node);
    rbtree_node *__begin(rbtree_node *start);
};

template <typename T, rbtree_node T::*hook, typename KeyT, KeyT T::*key>
struct malloc_rbtree: public malloc_rbtree_base {
private:
    using self_t = malloc_rbtree<T, hook, KeyT, key>;
    struct key_wrapper: malloc_rbtree_key_base {
       void dummy() {} 
       KeyT value;
    };

    T *container_of(rbtree_node *node) {
        return (T *)((uintptr_t)node - offset_of<T, rbtree_node, hook>());
    }
       
    bool less_than(rbtree_node *n1, rbtree_node *n2) override {
        if (!not_null(n1) || !not_null(n2)) __asm__("ud2");
        return container_of(n1)->*key < container_of(n2)->*key;
    }

    bool less_than(malloc_rbtree_key_base *k, rbtree_node *node) override {
        if (!not_null(node)) __asm__("ud2");
        return dynamic_cast<key_wrapper *>(k)->value < container_of(node)->*key;
    }

    bool less_than(rbtree_node *node, malloc_rbtree_key_base *k) override {
        if (!not_null(node)) __asm__("ud2");
        return container_of(node)->*key < dynamic_cast<key_wrapper *>(k)->value;
    }

public:
    bool insert(T *obj) {
        LockGuard<LOCK_TYPE> guard(lock); 
        return __insert(&(obj->*hook));
    }

    T *next(T *obj) {
        //LockGuard<LOCK_TYPE> guard(lock); 
        if (!obj) __asm__("ud2");
        auto next_node = __next(&(obj->*hook));
        
        if (!next_node) return nullptr;

        return container_of(next_node);
    }

    T *begin() {
        //LockGuard<LOCK_TYPE> guard(lock); 
        auto begin_node = __begin(root);
        
        if (!begin_node) return nullptr;

        return container_of(begin_node); 
    }

    T *erase(T *obj) {
        LockGuard<LOCK_TYPE> guard(lock); 
        auto ret_node = __erase(&(obj->*hook));

        if (!ret_node) return nullptr;

        return container_of(ret_node);
    }

    T *lookup(KeyT v) {
        //LockGuard<LOCK_TYPE> guard(lock); 
        key_wrapper temp;
        temp.value = v;
        
        auto ret_node = __lookup(&temp);

        if (!ret_node) return nullptr;

        return container_of(ret_node);
    }
};


}
