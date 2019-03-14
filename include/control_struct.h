#ifndef CONTROL_STRUCT_H
#define CONTROL_STRUCT_H

#include <sgx_arch.h>
#include <request.h>
#include <queue.h>
#include <spin_lock.h>
using namespace std;

#define CONTROL_STRUCT_MAGIC 0xbeefbeef

struct main_args_t {
    int argc;
    char **argv;
    vaddr heapBase;
    size_t heapLength;
};

struct slave_args_t {
    void *(* job)(void *);
    void *arg;
};

struct panic_struct {
   SpinLock lock;

   char panicBuf[1024];
   char requestBuf[sizeof(DebugRequest)];
};

struct libOS_control_struct {
   /* for debugging */
   unsigned int magic = CONTROL_STRUCT_MAGIC;

   bool isMain;

   union {
       main_args_t mainArgs;
       slave_args_t slaveArgs;
   };

   Queue<RequestBase> *requestQueue; 

   panic_struct *panic;
};

#endif
