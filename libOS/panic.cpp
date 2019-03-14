#include "panic.h"
#include <new>
#include <streambuf>
#include <request.h>
#include <control_struct.h>
#include "libos.h"

panic_struct *panicInfo = nullptr;
int panicCounter = 0;

using namespace std;
void writeToConsole(const char *msg, size_t n) {
    if (n >= PANIC_BUFFER_SIZE) n = PANIC_BUFFER_SIZE - 1;

    panicInfo->lock.lock();
    char *ptr = panicInfo->panicBuf;
    size_t i = 0;
    while (*msg && i < n) {
        *ptr = *msg;
        msg++;
        ptr++;
        i++;
    }
    *ptr = 0;
    
    auto req = new (panicInfo->requestBuf) DebugRequest;
    if (panicCounter > 0) requestQueue->debug = true;
    requestQueue->push(req); 
    req->waitOnDone(1000000000);
    req->requestType = 0;
    panicInfo->lock.unlock();
    panicCounter++;
}

extern "C" void libos_panic(const char *msg) {
   writeToConsole(msg, PANIC_BUFFER_SIZE - 1); 
//.   __asm__ ("ud2");   //commit suicide
}

class PanicStreamBuf : public std::streambuf {
public:
    PanicStreamBuf() {
        this->setp(this->buf, this->buf + PANIC_BUFFER_SIZE - 1);
    }
private:
    char buf[PANIC_BUFFER_SIZE - 1];
    
    virtual int sync() override {
        size_t nBytes = this->pptr() - this->pbase(); 
        writeToConsole(this->pbase(), nBytes);
        this->setp(this->pbase(), this->epptr());
        return 0;
    }

    virtual int overflow(int c) override {
        this->sync();
        if (c != EOF)
            this->sputc(c);
        return c;
    }
};

char panicStreamBuf[sizeof(PanicStreamBuf)];

void initPanic(panic_struct *ps) {
    panicInfo = ps;
    panicCounter = 0;
    // call the constructor manually
    // because we may not have called the init_array functions
    // TODO: currently running this causes a sigbus!
    //new (&panicStreamBuf) PanicStreamBuf;
}

std::streambuf *getPanicStreamBuf() {
    return (std::streambuf *)panicStreamBuf;
}

