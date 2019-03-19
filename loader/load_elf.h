#ifndef LOAD_ELF_H
#define LOAD_ELF_H

#include <memory>
#include "enclave_thread.h"

class EnclaveManager;

/* For now just leave it like this */
std::shared_ptr<EnclaveMainThread> load_one(const char *filename, shared_ptr<EnclaveManager> enclaveManager);
/* Should be like:
 *
 * bool load_one(shared_ptr<EnclaveManager>, const char *fname);
 *
 * It is because we need to load more than one files in case of 
 * dynamically linked libraries.
 */

std::shared_ptr<EnclaveMainThread> load_static(const char *filename, shared_ptr<EnclaveManager> enclaveManager, uint64_t enclaveLen = 0x400000);

extern atomic_flag __sig_flag;

#endif
