#ifndef LOAD_ELF_H
#define LOAD_ELF_H

#include <memory>

class EnclaveManager;

/* For now just leave it like this */
std::shared_ptr<EnclaveManager> load_one(const char *filename);

/* Should be like:
 *
 * bool load_one(shared_ptr<EnclaveManager>, const char *fname);
 *
 * It is because we need to load more than one files in case of 
 * dynamically linked libraries.
 */

std::shared_ptr<EnclaveManager> load_static(const char *filename);


#endif
