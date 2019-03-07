#ifndef REQUEST_H
#define REQUEST_H

#include <atomic>
using namespace std;

struct RequestBase {
    atomic_bool ack = false;
    atomic_bool done = false;
    int returnVal = 0;

};


#endif
