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
    void *unsafeHeapBase;
    size_t unsafeHeapLength;
};

struct slave_args_t {
    void *(* job)(void *);
    void *arg;
};

struct panic_struct {
   SpinLockNoTimer lock;

   char panicBuf[1024];
   struct {} __attribute__ ((aligned (16)));
   char requestBuf[sizeof(DebugRequest)];
   char requestTest[sizeof(DebugRequest)];
};

struct libOS_control_struct {
   /* for debugging */
   unsigned int magic = CONTROL_STRUCT_MAGIC;

   bool isMain;

   union {
       main_args_t mainArgs;
       slave_args_t slaveArgs;
   };

   Queue<RequestBase*> *requestQueue; 

   panic_struct *panic;
};

#endif
