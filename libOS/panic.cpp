#include "panic.h"
#include <new>
#include <request.h>
#include "libos.h"

char *panicBuffer;

char *requestBuf;

extern "C" void libos_panic(const char *msg) {
    char *ptr = panicBuffer;
    int i = 0;
    while (*msg && i < PANIC_BUFFER_SIZE - 1) {
        *ptr = *msg;
        msg++;
        ptr++;
        i++;
    }
    *ptr = 0;
    
    auto req = new (requestBuf) DebugRequest;
    requestQueue->push(req); 
    req->waitOnDone(1000000000);
    __asm__ ("ud2");   //commit suicide
}
