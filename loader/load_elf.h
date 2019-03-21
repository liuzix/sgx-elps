#ifndef LOAD_ELF_H
#define LOAD_ELF_H

#include <memory>
#include <elfio/elfio.hpp>
#include "enclave_thread.h"

using namespace ELFIO;
using namespace std;
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

class ELFLoader {
private:
    elfio reader;
    char *mappedFile;
    uint64_t loadBias;    
    std::shared_ptr<EnclaveManager> enclaveManager; 

    uint64_t memOffsetToFile(uint64_t memoff);
    bool mapFile(const string &filename);
    int64_t calculateBias();
public:
    bool open(const string &filename);
    bool relocate();
    shared_ptr<EnclaveMainThread> load();

    ELFLoader(std::shared_ptr<EnclaveManager> manager) :
        enclaveManager(manager) { }
};

#endif
