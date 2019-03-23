#pragma once
#include "sched.h"
#include <boost/context/detail/fcontext.hpp>
#include <functional>

#define STACK_SIZE 8192
using namespace std;
using namespace boost::context::detail;

class UserThread {
    fcontext_t fcxt;

    void start();
    void terminate();
public:
    function<int(void)> entry; 
    SchedEntity se; 
    int id;
    void jumpTo();

    /* for creating new thread */
    UserThread(function<int(void)> _entry);
};
