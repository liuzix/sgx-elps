#include "panic.h"
#include <string.h>
#include <new>
#include <streambuf>
#include <request.h>
#include <control_struct.h>
#include "libos.h"

panic_struct *panicInfo = nullptr;

using namespace std;
void writeToConsole(const char *msg) {
    strcpy(panicInfo->panicBuf, msg);
    auto req = new (panicInfo->requestBuf) DebugRequest;
    req->printBuf = panicInfo->panicBuf;
    requestQueue->push(req); 
    req->waitOnDone(1000000000);
    panicInfo->lock.unlock();
}

extern "C" void libos_panic(const char *msg) {
    writeToConsole(msg); 
}

void initPanic(panic_struct *ps) {
    panicInfo = ps;
}


