#include "panic.h"
#include <string.h>
#include <new>
#include <streambuf>
#include <request.h>
#include <control_struct.h>
#include "thread_local.h"
#include "libos.h"

panic_struct *panicInfo = nullptr;

using namespace std;

void strcpy2(char dest[], const char source[]) 
{
    int i = 0;
    while ((dest[i] = source[i]) != '\0')
    {
        i++;
    } 
}

void writeToConsole(const char *msg) {
    panicInfo->lock.lock();
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

