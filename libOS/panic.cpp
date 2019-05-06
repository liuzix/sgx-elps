#include "panic.h"
#include <string.h>
#include <cstdlib>
#include <new>
#include <streambuf>
#include <request.h>
#include <control_struct.h>
#include <swapper.h>
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
    sprintf(panicInfo->panicBuf, "[%ld]%s", getSharedTLS()->threadID, msg);
    auto req = new (panicInfo->requestBuf) DebugRequest;
    req->printBuf = panicInfo->panicBuf;
    requestQueue->push(req);
    req->waitOnDone(1000000000);
    panicInfo->lock.unlock();
/*
    char tbuf[512] = {'\0'};
    int res;
    char *debuggerThreadBuf;
    int buf_index;

    panicInfo->lock.lock();
    debuggerThreadBuf = getSharedTLS()->dbg_buffer;
    buf_index = *(getSharedTLS()->dbg_pbuffer_index);
    res = snprintf(tbuf, 512, "[%ld]%s\n", getSharedTLS()->threadID, msg);
    if (res >= DBBUF_SIZE - buf_index) {
        while (!debuggerThreadBuf) ;
        buf_index = 0;
    }
    memcpy(debuggerThreadBuf + buf_index, tbuf, res);
    buf_index += res;
    if (buf_index >= DBBUF_SIZE)
        buf_index = 0;
    *(getSharedTLS()->dbg_pbuffer_index) = buf_index;
    panicInfo->lock.unlock();
*/
}

extern "C" void libos_panic(const char *msg) {
    writeToConsole(msg);
}

void initPanic(panic_struct *ps) {
    panicInfo = ps;
}

