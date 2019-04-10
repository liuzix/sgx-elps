#pragma once
#include "sched.h"
#include <boost/context/detail/fcontext.hpp>
#include <functional>

#define STACK_SIZE 8192
using namespace std;
using namespace boost::context::detail;

class UserThread {
    void start();
    void terminate();
public:
    fcontext_t fcxt;
    uint64_t preempt_stack;
    function<int(void)> entry;
    SchedEntity se;
    int id;
    void jumpTo(UserThread *from);

    /* for creating new thread */
    UserThread(function<int(void)> _entry);
};
