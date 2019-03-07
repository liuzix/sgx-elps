#include "enclave_thread.h"
#include "logging.h"

void EnclaveThread::run() {
    console->info("entering enclave!");
    int ret = __eenter(this->tcs, this->stack, nullptr);
    console->info("returned from enclave! ret = {}", ret);
}
