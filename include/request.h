#ifndef REQUEST_H
#define REQUEST_H

#include <atomic>
#include <functional>
#include <unordered_map>
#include <cassert>
#include <logging.h>
#include <queue.h>
#include <boost/intrusive/list.hpp>
#include <syscall_format.h>
#include "../libOS/allocator.h"
#include "../libOS/linked_list.h"
using namespace std;
extern uint64_t __jiffies;
class UserThread;

class RequestBase {
private:
    atomic_bool ack = {false};
    int returnVal = 0;
public:
    atomic_bool done = {false};
    UserThread *owner = nullptr;
    //boost::intrusive::list_member_hook<> watchListHook;
    ListNode watchListHook;
    int requestType;
    bool waitOnAck(uint32_t cycles) {
        for (uint32_t i = 0; i < cycles; i++)
            if (ack.load()) return true;
        return false;
    }

    void blockOnDone() {
        for (;;)
            if (done.load()) return;
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
        __sync_synchronize(); 
        asm volatile("": : :"memory");
        done.store(true);
        asm volatile("": : :"memory");
        __sync_synchronize(); 
    }

    RequestBase() {}
};

class DebugRequest: public RequestBase {
public:
    constexpr static int typeTag = 0;
    enum class SubType { enclavePrint, enclaveCallDebugger, enclaveExit };

    char *printBuf;
    DebugRequest() {
        this->requestType = typeTag;
    }
};

class SwapRequest: public RequestBase {
public:
    constexpr static int typeTag = 3;
    unsigned long addr;
    SwapRequest() {
        this->requestType = typeTag;
    }
};

class SleepRequest: public RequestBase {
public:
    constexpr static int typeTag = 4;
    unsigned long ns;
    SleepRequest(unsigned long _ns) {
        this->requestType = typeTag;
        ns = _ns;
    }
};

class SyscallRequest: public RequestBase {
public:
    constexpr static int typeTag = 1;
    long sys_ret;
    format_t fm_list;
    /* unsigned int syscall_num
     * unsigned int args_num
     * unsigned int types[6]
     * unsigned int sizes[6]
     */
    syscall_arg_t args[6];
    /* long arg
     * char *data
     */
    SyscallRequest() {
        this->requestType = typeTag;
    }
    ~SyscallRequest() {
        for (int i = 0; i < 6; i++)
            if (args[i].data != nullptr) {
                deepClean(i);
                unsafe_slub_free(args[i].data);
            }
    }
    void deepClean(int i);
    bool fillArgs();
    void fillEnclave(long* enclave_args);
};

class SchedulerRequest: public RequestBase {
public:
    constexpr static int typeTag = 2;
    enum class SchedulerRequestType { NewThread, SchedReady, DecThread };

    SchedulerRequestType subType;
    SchedulerRequest(SchedulerRequestType t) {
        this->requestType = SchedulerRequest::typeTag;
        subType = t;
    }
};

#ifndef IS_LIBOS
template <typename RequestType>
RequestType *tryCast(RequestBase *basePtr) {
    if (basePtr->requestType == RequestType::typeTag)
        return static_cast<RequestType *>(basePtr);
    else
        return nullptr;
}

class RequestDispatcher {
private:
    unordered_map<int, function<void(RequestBase *)>> handlers;  
    string __name;
    DEFINE_MEMBER_LOGGER(__name, spdlog::level::trace);
public:
    RequestDispatcher(int i) { __name = "RequestDispatcher" + i; }
    template <typename RequestType>
    void addHandler(function<void(RequestType *)> handler) {
        auto realHandler = [=](RequestBase *basePtr) {
            auto subTypePtr = tryCast<RequestType>(basePtr);
            assert(subTypePtr);
            handler(subTypePtr);
        };
        handlers[RequestType::typeTag] = realHandler;
    }

    void dispatch(RequestBase *basePtr) {
        //uint64_t jiffies = 0;
        //if (basePtr->requestType == 3)
        //    jiffies = __jiffies;
        if (handlers.count(basePtr->requestType) != 1) {
            classLogger->critical("Unknown request type {} at 0x{:x}",
                    basePtr->requestType, (uint64_t)basePtr);
            __asm__("ud2");
        }
        basePtr->setAck();
        //if (basePtr->requestType == 3)
            //classLogger->info("ack jiffies: {}", __jiffies - jiffies);
        handlers[basePtr->requestType](basePtr);
        //if (basePtr->requestType == 3)
         //   jiffies = __jiffies;
        //basePtr->setDone();
        //if (basePtr->requestType == 3)
            //classLogger->info("done jiffies: {}", __jiffies - jiffies);
    }
};
#endif
#endif
