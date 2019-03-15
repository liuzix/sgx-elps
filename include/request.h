#ifndef REQUEST_H
#define REQUEST_H

#include <queue.h>
#include <atomic>

using namespace std;

class RequestBase {
private:
    atomic_bool ack = {false};
    atomic_bool done = {false};
    int returnVal = 0;
public:
    int requestType;
    bool waitOnAck(uint32_t cycles) {
        for (uint32_t i = 0; i < cycles; i++)
            if (ack.load()) return true;
        return false;
    }

    bool waitOnDone(uint32_t cycles) {
        for (uint32_t i = 0; i < cycles; i++)
            if (done.load()) return true;
        return false;
    }

    void setAck() {
        ack.store(true);
    }

    void setDone() {
        done.store(true);
    }

    RequestBase() {}
};

class DebugRequest: public RequestBase { 
    const static int typeTag = 0;
public:
    static bool isInstanceOf(RequestBase *req) {
        return req->requestType == typeTag;
    }

    DebugRequest() {
        this->requestType = typeTag;
    }
};

#endif
