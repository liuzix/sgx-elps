#pragma once
#include <boost/context/detail/fcontext.hpp>
#include <functional>

#define STACK_SIZE 8192
using namespace std;
using namespace boost::context::detail;

class UserThread {
    fcontext_t fcxt;
    function<int(void)> entry; 

    void start();
    void terminate();
public:
    void jumpTo();

    /* for creating new thread */
    UserThread(function<int(void)> _entry);

    friend void __entry_helper(transfer_t transfer);
};
