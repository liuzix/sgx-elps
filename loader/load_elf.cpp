#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>

#include <elfio/elfio.hpp>
#include "logging.h"
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

shared_ptr<EnclaveMainThread> load_one(const char *filename, shared_ptr<EnclaveManager> enclaveManager) {
    return load_static(filename, enclaveManager);
}

shared_ptr<EnclaveMainThread> load_static(const char *filename, shared_ptr<EnclaveManager> enclaveManager, uint64_t enclaveLen) {
    elfio reader = elfReadAndCheck(filename);
    Elf_Half seg_num = reader.segments.size();
    uint64_t bias = 0;
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
            console->trace("ELF load: vaddr = 0x{:x}, p_filesz = 0x{:x}, p_memsz = 0x{:x}",
                           p_vaddr, p_filesz, p_memsz);
            Elf64_Word p_flags = pseg->get_flags();
            off_t p_offset = pseg->get_offset();
            off_t mapoff = p_offset & pageshift;
            int prot = (p_flags & PF_R ? PROT_READ : 0) |
                       (p_flags & PF_W ? PROT_WRITE : 0) |
                       (p_flags & PF_X ? PROT_EXEC : 0);
            void *base = 0;
            Elf64_Addr allocbegin, allocend;
            Elf64_Addr dataend = p_vaddr + p_filesz;
            void *t;

            allocbegin = p_vaddr & pagemask;
            /* Padding p_memsz to page size */
            //if (p_memsz % PRESET_PAGESIZE != 0)
            //    p_memsz += PRESET_PAGESIZE - (p_memsz % PRESET_PAGESIZE);
            allocend = p_vaddr + p_memsz;
            allocend = (allocend + pageshift) & ~pageshift;
            console->trace("allocbegin = 0x{:x}, allocend = 0x{:x}", allocbegin, allocend);

            if (!enclaveManager)
                enclaveManager = make_shared<EnclaveManager>(p_vaddr, enclaveLen);

            if (!bias && enclaveManager->getBase() > (uint64_t)allocbegin) {
                bias = enclaveManager->getBase() - allocbegin;
                allocbegin += bias;
                allocend += bias;
            }

            if (p_vaddr < enclaveManager->getBase()) {
                console->critical("Bad base address 0x{:x}, first load 0x{:x}",
                                  enclaveManager->getBase(), p_vaddr);
                goto out;
            }

            base = mmap(NULL, allocend - allocbegin, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            memset(base, 0, allocend - allocbegin);
            if (IS_ERR_P(base)) {
                enclaveManager = nullptr;
                goto out;
            }

            /* Copy the segment data */
            memcpy((char *)base + mapoff, pseg->get_data(), p_filesz);


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
            console->info("allocTotalLen = 0x{:x}", allocend - allocbegin);
            enclaveManager->addPages(allocbegin, base, allocend - allocbegin);

            break;
        }
    }

    return enclaveManager->createThread<EnclaveMainThread>((vaddr)reader.get_entry());
out:
    return nullptr;
}
