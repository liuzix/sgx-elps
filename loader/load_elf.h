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
    struct auxv_t{
        uint64_t entry;
        uint64_t phdr;
        uint64_t phnum;
        uint64_t phent;
        uint64_t fd;
    } aux_v;
public:
    bool open(const string &filename);
    bool relocate();
    shared_ptr<EnclaveMainThread> load();
    shared_ptr<EnclaveThread> makeWorkerThread();

    ELFLoader(std::shared_ptr<EnclaveManager> manager) :
        enclaveManager(manager) { }


    void setAuxEntry(uint64_t entry) { this->aux_v.entry = entry; }
    void setAuxPhdr(uint64_t phdr) { this->aux_v.phdr = phdr; }
    void setAuxPhnum(uint64_t phnum) { this->aux_v.phnum = phnum; }
    void setAuxPhent(uint64_t phent) { this->aux_v.phent = phent; }
    void setAuxFd(uint64_t fd) { this->aux_v.fd = fd; }

    uint64_t getAuxEntry()  { return this->aux_v.entry; }
    uint64_t getAuxPhdr() { return this->aux_v.phdr; }
    uint64_t getAuxPhnum() { return this->aux_v.phnum; }
    uint64_t getAuxPhent() { return this->aux_v.phent; }
    uint64_t getAuxFd() { return this->aux_v.fd; }
    uint64_t getBias() { return this->loadBias; }

};

#endif
