#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <elfio/elfio.hpp>
#include "enclave_manager.h"

using namespace ELFIO;

#define PRESET_PAGESIZE (1 << 12)

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#undef INTERNAL_SYSCALL_ERROR_P
#define INTERNAL_SYSCALL_ERROR_P(val) \
  ((unsigned long) (val) >= (unsigned long)-4095L)

#define IS_ERR_P INTERNAL_SYSCALL_ERROR_P


unsigned long pagemask  = ~(PRESET_PAGESIZE - 1);
unsigned long pageshift = PRESET_PAGESIZE - 1;

int load_static(const char *filename)
{
	elfio reader;
	int ret, i;

	if ( !reader.load( filename ) ) {
		std::cout << "Can't find or process ELF file "
			  << filename << std::endl;
		return 2;
	}

	Elf_Half seg_num = reader.segments.size();

	auto enclaveManager = new EnclaveManager((vaddr)NULL, 0x80000);

	for (i = 0; i < seg_num; i++) {
		const segment* pseg = reader.segments[i];

		switch (pseg->get_type()) {
		case PT_DYNAMIC:
			//TODO: dyn
			break;
		case PT_LOAD:
			Elf64_Addr p_vaddr = pseg->get_virtual_address();
			Elf_Xword p_filesz = pseg->get_file_size();
			Elf_Xword p_memsz = pseg->get_memory_size();
			Elf64_Word p_flags = pseg->get_flags();
			off_t p_offset = pseg->get_offset();
			off_t mapoff = p_offset & pagemask;
			int prot =(p_flags & PF_R ? PROT_READ  : 0) |
				  (p_flags & PF_W ? PROT_WRITE : 0) |
				  (p_flags & PF_X ? PROT_EXEC  : 0);
			void *base = 0;
			Elf64_Addr allocend = p_vaddr + p_memsz;
			Elf64_Addr dataend = p_vaddr + p_filesz;
			void *t;

			base = mmap(NULL, p_memsz, PROT_READ | PROT_WRITE,
				    MAP_PRIVATE | MAP_ANONYMOUS, -1, mapoff);
			if (IS_ERR_P(base)) {
				ret = errno;
				goto out;
			}
			/* Copy the segment data */
			memcpy(base, pseg->get_data(), p_memsz);
#ifdef __DEBUG__
			std::ofstream f;
			f.open("dump.data", std::ios::out|std::ios::binary|std::ios::app);
			f.write((const char *)base, p_memsz);
			f.close();
#endif
			/* Fill unused memory with 0*/

			if (allocend <= dataend)
				goto do_map;
			unsigned long zero, zeroend, zeropage;

			zero = (unsigned long)base + p_filesz;
			zeroend = ((unsigned long)base + (unsigned long)p_memsz +
				   pageshift) & pagemask;
			zeropage = ((unsigned long)zero + pageshift) & pagemask;

			if (zeroend < zeropage)
				zeropage = zeroend;
#ifdef __DEBUG_
			std::cout << "zeropage: " << std::hex << zeropage << std::endl;
			std::cout << "zeroend: " << std::hex << zeroend << std::endl;
			std::cout << "dataend: " << std::hex << dataend << std::endl;
			std::cout << "memsz: " << std::hex << p_memsz << std::endl;
			std::cout << "base: " << std::hex <<(unsigned long)base << std::endl;
			std::cout << "zero: " << std::hex << zero << std::endl;
#endif

			if (zeropage > zero)
				memset((void *) zero, 0, zeropage - zero);

			if (zeroend <= zeropage)
				goto do_map;

			t = mmap((void *)zeropage, zeroend - zeropage,
				prot, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
			if (IS_ERR_P(t)) {
				ret = errno;
				goto out;
			}
			/* Add pages to SGX*/
do_map:
			mprotect(base, p_memsz, prot);
#ifdef __DEBUG__
			f.open("padded_dump.data", std::ios::out|std::ios::binary|std::ios::app);
			f.write((const char *)base, p_memsz);
			f.close();
#endif
			enclaveManager->addPages(p_vaddr, base, p_memsz);

			break;
		}
	}

out:
	return ret;
}

#ifdef __DEBUG__
int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cout << "usage: ./a.out [bianry_file]" << std::endl;
		return -1;
	}
	load_static(argv[1]);
	return 0;
}
#endif
