#pragma once
#include <functional>

#define STACK_SIZE 8192
using namespace std;

class UserThread {
    void *sp;
    function<int(void)> entry;    

    void start();
    void terminate();
public:
    void jumpTo();

    /* for creating new thread */
    UserThread(function<int(void)> _entry);

    /* for transforming current context into a thread */
    UserThread();
};
