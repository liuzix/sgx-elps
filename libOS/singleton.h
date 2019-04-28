#ifndef SINGLETON_H
#define SINGLETON_H

#include <unordered_map>
#include "request.h"
#include "sched.h"
#include "user_thread.h"
#include <cstddef>
#include <iostream>
#include "allocator.h"
#include "panic.h"

template <typename ReqT>
class Singleton {
private:
    static std::unordered_map<int, ReqT*> *umap;
    Singleton() {}
public:
    //ReqT* getRequest(void *addr);
    template <typename... T>
    static ReqT* getRequest(T&&... args) {
        ReqT* req;
        if (scheduler->current.get() == nullptr) {
            ReqT *tmp = (ReqT *)unsafeMalloc(sizeof(ReqT));
            req = new (tmp) ReqT(std::forward<T>(args)...);
            return req;
        }
        int tid = (**scheduler->getCurrent())->thread->id;
        if (umap == nullptr)
            umap = new std::unordered_map<int, ReqT*>();
        if ((*umap)[tid] == nullptr) {
            ReqT *tmp = (ReqT *)unsafeMalloc(sizeof(ReqT));
            req = new (tmp) ReqT(std::forward<T>(args)...);
        }
        else
            req = new ((*umap)[tid]) ReqT(std::forward<T>(args)...);

        (*umap)[tid] = req;

        return req;
    }
};


#endif
