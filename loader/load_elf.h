#ifndef LOAD_ELF_H
#define LOAD_ELF_H

#include <memory>

class EnclaveManager;

/* For now just leave it like this */
unique_ptr<EnclaveThread> load_one(const char *filename, shared_ptr<EnclaveManager> enclaveManager);
/* Should be like:
 *
 * bool load_one(shared_ptr<EnclaveManager>, const char *fname);
 *
 * It is because we need to load more than one files in case of 
 * dynamically linked libraries.
 */

unique_ptr<EnclaveThread> load_static(const char *filename, shared_ptr<EnclaveManager> enclaveManager);

#endif
