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
    static SpinLock *mapLock;
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
        if (umap == nullptr) {
            mapLock = new SpinLock;
            umap = new std::unordered_map<int, ReqT*>();
        }
        mapLock->lock();
        if ((*umap)[(**scheduler->getCurrent())->thread->id] == nullptr) {
            ReqT *tmp = (ReqT *)unsafeMalloc(sizeof(ReqT));
            req = new (tmp) ReqT(std::forward<T>(args)...);
        }
        else
            req = new ((*umap)[(**scheduler->getCurrent())->thread->id]) ReqT(std::forward<T>(args)...);
            //req = new ((ReqT *)(**scheduler->getCurrent())->thread->request_obj) ReqT(std::forward<T>(args)...);
        (*umap)[(**scheduler->getCurrent())->thread->id] = req;
        mapLock->unlock();
        return req;
    }
};


#endif
