#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "elf.h"

/* We use this macro to refer to ELF types independent of the native wordsize.
   `ElfW(TYPE)' is used in place of `Elf32_TYPE' or `Elf64_TYPE'.  */
#define ElfW(type)	_ElfW (Elf, __ELF_NATIVE_CLASS, type)
#define _ElfW(e,w,t)	_ElfW_1 (e, w, _##t)
#define _ElfW_1(e,w,t)	e##w##t

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

int load_static(const char *filename, void **load_addr, void **entry,
		 ElfW(Dyn) **dyn, unsigned long *phoff, int *phnum)
{
	int fd = open(filename, O_RDONLY|O_CLOEXEC);
	char filebuf[512];
	int maplength, nloadcmds = 0, ret;

	if (fd == -1) {
		ret = errno;
		printf("file open failed.\n");
		return ret;
	}

	ret = read(fd, filebuf, 512);
	if (ret == -1) {
		ret = errno;
		printf("file read failed.\n");
		goto out;
	}
	const ElfW(Ehdr) * header = (void *) filebuf;
	const ElfW(Phdr) * phdr = (void *) filebuf + header->e_phoff;
	const ElfW(Phdr) * ph;

	ElfW(Addr) base = 0;

	*phoff = header->e_phoff;
	*phnum = header->e_phnum;

	struct loadcmd {
		ElfW(Addr) mapstart, mapend, dataend, allocend;
		off_t mapoff;
		int prot;
	} loadcmds[16], *c;

	for (ph = phdr ; ph < &phdr[header->e_phnum] ; ph++)
		switch (ph->p_type) {
		case PT_DYNAMIC:
			*dyn = (void *) ph->p_vaddr;
			break;

		case PT_LOAD:
			if (nloadcmds == 16) {
			ret = -EINVAL;
			goto out;
			}

			c = &loadcmds[nloadcmds++];
			c->mapstart = ph->p_vaddr & pagemask;
			c->mapend = (ph->p_vaddr + ph->p_filesz + pageshift)
				    & pagemask;
			c->dataend = ph->p_vaddr + ph->p_filesz;
			c->allocend = ph->p_vaddr + ph->p_memsz;
			c->mapoff = ph->p_offset & pagemask;
			c->prot = (ph->p_flags & PF_R ? PROT_READ  : 0) |
				  (ph->p_flags & PF_W ? PROT_WRITE : 0) |
				  (ph->p_flags & PF_X ? PROT_EXEC  : 0);
			printf("----------load section--------------\n");
			printf("pagemask = %lx\n", pagemask);
			printf("pageshift = %lx\n", pageshift);
			printf("mapstart = %lx\n", (unsigned long)c->mapstart);
			printf("mapend = %lx\n", (unsigned long)c->mapend);
			printf("dataend = %lx\n", (unsigned long)c->dataend);
			printf("allocend = %lx\n", (unsigned long)c->allocend);
			printf("mapoff = %lx\n", (unsigned long)c->mapoff);
			printf("prot = %d\n", c->prot);
			break;
		}

	c = loadcmds;
	maplength = loadcmds[nloadcmds - 1].allocend - c->mapstart;

	base = (ElfW(Addr))mmap(NULL, maplength, c->prot, MAP_PRIVATE | MAP_FILE,
		 fd, c->mapoff);
	if (IS_ERR_P(base)) {
		ret = errno;
		goto out;
	}

	goto postmap;

	for ( ; c < &loadcmds[nloadcmds] ; c++) {
		ElfW(Addr) addr = (ElfW(Addr))mmap((void *)base + c->mapstart,
		   c->mapend - c->mapstart, c->prot,
		   MAP_PRIVATE|MAP_FILE|MAP_FIXED,
		   fd, c->mapoff);
		if (IS_ERR_P(addr)) {
			ret = errno;
			goto out;
		}

postmap:
		if (c == loadcmds)
			munmap((void *)base + c->mapend, maplength - c->mapend);

		if (c->allocend <= c->dataend)
			continue;

		ElfW(Addr) zero, zeroend, zeropage;

		zero = base + c->dataend;
		zeroend = (base + c->allocend + pageshift) & pagemask;
		zeropage = (zero + pageshift) & pagemask;

		if (zeroend < zeropage)
			zeropage = zeroend;

		if (zeropage > zero)
			memset((void *) zero, 0, zeropage - zero);

		if (zeroend <= zeropage)
			continue;

		addr = (ElfW(Addr))mmap((void *)zeropage, zeroend - zeropage,
			c->prot, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
		if (IS_ERR_P(addr)) {
			ret = errno;
			goto out;
		}
	}

	*dyn = (void *) (base + (ElfW(Addr)) *dyn);
	*load_addr = (void *) base;
	*entry = (void *) base + header->e_entry;

out:
	close(fd);
	return ret;
}

