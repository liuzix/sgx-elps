#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>

#include <elfio/elfio.hpp>
#include <logging.h>

#include "enclave_manager.h"
#include "load_elf.h"

#define LOAD_ELF_DUMP

using namespace std;
using namespace ELFIO;

#define PRESET_PAGESIZE (1 << 12)

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#undef INTERNAL_SYSCALL_ERROR_P
#define INTERNAL_SYSCALL_ERROR_P(val)                                          \
    ((unsigned long)(val) >= (unsigned long)-4095L)

#define IS_ERR_P INTERNAL_SYSCALL_ERROR_P

const unsigned long pagemask = ~(PRESET_PAGESIZE - 1);
const unsigned long pageshift = PRESET_PAGESIZE - 1;

static elfio elfReadAndCheck(const string &filename) {
    elfio reader;

    if (!reader.load(filename)) {
        console->error("Unable to load {}", filename);
        exit(-1);
    }

    if (reader.get_class() != ELFCLASS64 || reader.get_machine() != EM_X86_64) {
        console->error("Unsupported architecture");
        exit(-1);
    }

    return reader;
}

shared_ptr<EnclaveManager> load_one(const char *filename) {
    return load_static(filename);
}

shared_ptr<EnclaveManager> load_static(const char *filename) {
    elfio reader = elfReadAndCheck(filename);
    Elf_Half seg_num = reader.segments.size();
    shared_ptr<EnclaveManager> enclaveManager = nullptr;

    int i;
    for (i = 0; i < seg_num; i++) {
        const segment *pseg = reader.segments[i];

        switch (pseg->get_type()) {
        case PT_DYNAMIC:
            // TODO: dyn
            break;
        case PT_LOAD:
            Elf64_Addr p_vaddr = pseg->get_virtual_address();
            Elf_Xword p_filesz = pseg->get_file_size();
            Elf_Xword p_memsz = pseg->get_memory_size();
            Elf64_Word p_flags = pseg->get_flags();
            off_t p_offset = pseg->get_offset();
            off_t mapoff = p_offset & pagemask;
            int prot = (p_flags & PF_R ? PROT_READ : 0) |
                       (p_flags & PF_W ? PROT_WRITE : 0) |
                       (p_flags & PF_X ? PROT_EXEC : 0);
            void *base = 0;
            Elf64_Addr allocend = p_vaddr + p_memsz;
            Elf64_Addr dataend = p_vaddr + p_filesz;
            void *t;

            if (!enclaveManager)
                enclaveManager = make_shared<EnclaveManager>(p_vaddr, 0x400000);

            if (p_vaddr < enclaveManager->getBase()) {
                console->critical("Bad base address 0x{:x}, first load 0x{:x}",
                                  enclaveManager->getBase(), p_vaddr);
                goto out;
            }
            base = mmap(NULL, p_memsz, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, mapoff);
            if (IS_ERR_P(base)) {
                enclaveManager = nullptr;
                goto out;
            }
            /* Copy the segment data */
            memcpy(base, pseg->get_data(), p_filesz);
#ifdef LOAD_ELF_DUMP
            std::ofstream f;
            f.open("dump.data",
                   std::ios::out | std::ios::binary | std::ios::app);
            f.write((const char *)base, p_memsz);
            f.close();
#endif
            /* Fill unused memory with 0*/

            if (allocend <= dataend)
                goto do_map;
            unsigned long zero, zeroend, zeropage;

            zero = (unsigned long)base + p_filesz;
            zeroend =
                ((unsigned long)base + (unsigned long)p_memsz + pageshift) &
                pagemask;
            zeropage = ((unsigned long)zero + pageshift) & pagemask;

            if (zeroend < zeropage)
                zeropage = zeroend;

            // TODO: these should be console->trace
            std::cout << "zeropage: " << std::hex << zeropage << std::endl;
            std::cout << "zeroend: " << std::hex << zeroend << std::endl;
            std::cout << "dataend: " << std::hex << dataend << std::endl;
            std::cout << "memsz: " << std::hex << p_memsz << std::endl;
            std::cout << "base: " << std::hex << (unsigned long)base
                      << std::endl;
            std::cout << "zero: " << std::hex << zero << std::endl;

            if (zeropage > zero)
                memset((void *)zero, 0, zeropage - zero);

            if (zeroend <= zeropage)
                goto do_map;

            t = mmap((void *)zeropage, zeroend - zeropage, prot,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
            if (IS_ERR_P(t)) {
                enclaveManager = nullptr;
                goto out;
            }
            /* Add pages to SGX*/
        do_map:
            /* not necessary */
            // mprotect(base, p_memsz, prot);

#ifdef LOAD_ELF_DUMP
            f.open("padded_dump.data",
                   std::ios::out | std::ios::binary | std::ios::app);
            f.write((const char *)base, p_memsz);
            f.close();
#endif
            // TODO: might want to set protection according to load segments
            enclaveManager->addPages(p_vaddr, base, p_memsz);

            break;
        }
    }
out:
    return enclaveManager;
}
