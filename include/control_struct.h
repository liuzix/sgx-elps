#ifndef CONTROL_STRUCT_H
#define CONTROL_STRUCT_H

#include <request.h>
#include <queue.h>
using namespace std;

#define CONTROL_STRUCT_MAGIC 0xbeefbeef

struct libOS_control_struct {
   /* for debugging */
   unsigned int magic = CONTROL_STRUCT_MAGIC;

   bool is_main;

   union {
       struct {
           int argc;
           char **argv;
       } main_args;
      
       /* just placeholder */
       void *worker_thread_ctrl;
   };

   Queue<RequestBase *> *requestQueue; 
};

#endif
