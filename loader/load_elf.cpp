#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <elfio/elfio.hpp>
#include <elfio/elf_types.hpp>
#include "logging.h"
#include "enclave_manager.h"
#include "load_elf.h"

DEFINE_LOGGER(ELFLoader, spdlog::level::trace)
using namespace std;
using namespace ELFIO;

#define PAGESIZE 4096
const unsigned long pagemask = ~(PAGESIZE - 1);
const unsigned long pageshift = PAGESIZE - 1;

bool ELFLoader::open(const string &filename) {
    if (!this->reader.load(filename)) {
        console->error("Unable to load {}", filename);
        return false;
    }

    if (reader.get_class() != ELFCLASS64 || reader.get_machine() != EM_X86_64) {
        console->error("Unsupported architecture");
        return false;
    }
    
    return this->mapFile(filename);
}

bool ELFLoader::mapFile(const string &filename) {
    int fd = ::open(filename.c_str(), O_RDWR);
    struct stat filestat;
    
    if (fstat(fd, &filestat) != 0) {
        console->error("mapFile: Cannot do fstat");
        return false;
    }

    this->mappedFile = (char *)mmap(NULL, filestat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (this->mappedFile == MAP_FAILED) {
        console->error("mapFile: Cannot do mmap");
        return false;
    }

    return true;
}

int64_t ELFLoader::calculateBias() {
    segment *pseg = nullptr;
    for (auto seg: reader.segments) {
        if (seg->get_type() == PT_LOAD) {
            pseg = seg;
            break;
        }
    }
    if (pseg == nullptr) {
        console->error("Sanity check failed: no segments in ELF");
        exit(-1);
    }

    vaddr enclaveBase = this->enclaveManager->getBase();
    int64_t bias = enclaveBase - pseg->get_virtual_address();

    console->info("Load bias is 0x{:x}", bias);
    this->loadBias = bias;
    return bias;
}

uint64_t ELFLoader::memOffsetToFile(uint64_t memoff) {
    for (auto seg: reader.segments) {
        if (seg->get_virtual_address() <= memoff
            && seg->get_virtual_address() + seg->get_memory_size() > memoff) {
            return seg->get_offset() + memoff - seg->get_virtual_address();
        }
    }
    console->error("cannot translate address 0x{:x}", memoff);
    return 0;
}

bool ELFLoader::relocate() {
    this->calculateBias();
    assert(mappedFile != MAP_FAILED); 
    section *rela = nullptr;
    for (auto &sec: reader.sections) {
       if (sec->get_type() == SHT_RELA) {
           console->info("Found relocation section: name {}", sec->get_name());
           rela = sec;
       }
    }

    if (rela == nullptr) {
        console->critical("Relocation section not found");
    }

    relocation_section_accessor accessor(reader, rela);
    for (unsigned long i = 0; i < accessor.get_entries_num(); i++) {
        Elf64_Addr offset;
        Elf_Word symbol;
        Elf_Word type;
        Elf_Sxword addend;
        
        accessor.get_entry(i, offset, symbol, type, addend);
        console->debug("relocation: offset {:x}, symbol {:x}, type {:x}, addend {:x}, image addr {:x}",
                offset, symbol, type, addend, offset + this->loadBias);
        if (type == R_X86_64_RELATIVE) {
            uint64_t *loc = (uint64_t *)(mappedFile + this->memOffsetToFile(offset));
            *loc = (uint64_t)addend + this->loadBias; 
            console->debug("relocated to 0x{:x}", *loc);
        } else {
            console->critical("Unknown relocation type {}", type);
        }
    }
    return true;
}

shared_ptr<EnclaveMainThread> ELFLoader::load() {

    Elf_Half seg_num = reader.segments.size();
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
            off_t p_offset = pseg->get_offset();
            off_t mapoff = p_offset & pageshift;
            void *base = 0;
            Elf64_Addr allocbegin, allocend;

            allocbegin = p_vaddr & pagemask;
            allocend = p_vaddr + p_memsz;
            allocend = (allocend + pageshift) & ~pageshift;
            
            allocbegin += this->loadBias;
            allocend += this->loadBias;
            console->trace("allocbegin = 0x{:x}, allocend = 0x{:x}", allocbegin, allocend);

            base = mmap(NULL, allocend - allocbegin, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (base == MAP_FAILED) {
                console->error("load_elf: mmap failed");
                exit(-1);
            }
            memset(base, 0, allocend - allocbegin);

            //*(uint64_t *)(loadBias + reader.get_entry()) = 0;
            /* Copy the segment data */
            memcpy((char *)base + mapoff, mappedFile + pseg->get_offset(), p_filesz);
            console->info("allocTotalLen = 0x{:x}", allocend - allocbegin);
            enclaveManager->addPages(allocbegin, base, allocend - allocbegin);

            munmap(base, allocend - allocbegin);

            break;
        }
    }

    auto ret = enclaveManager->createThread<EnclaveMainThread>((vaddr)reader.get_entry() + this->loadBias);
    ret->setBias(this->loadBias);
    return ret;
}

