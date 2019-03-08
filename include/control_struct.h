#ifndef CONTROL_STRUCT_H
#define CONTROL_STRUCT_H

#include <request.h>
#include <queue.h>
using namespace std;

#define CONTROL_STRUCT_MAGIC 0xbeefbeef

struct main_args_t {
    int argc;
    char **argv;
};

struct slave_args_t {
    void *(* job)(void *);
    void *arg;
};

struct libOS_control_struct {
   /* for debugging */
   unsigned int magic = CONTROL_STRUCT_MAGIC;

   bool isMain;

   union {
       main_args_t mainArgs;
       slave_args_t slaveArgs;
   };

   Queue<RequestBase *> *requestQueue; 
};

#endif
