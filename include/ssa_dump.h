#ifndef SSA_DUMP_H
#define SSA_DUMP_H
#include <csignal>
#include <ucontext.h>
#include <map>
#include <atomic>
#include <sgx_arch.h>

extern "C" char get_flag(uint64_t rbx)  __attribute__ ((visibility ("hidden")));
extern void set_flag(uint64_t rbx, char flag);
extern "C" void sig_exit()  __attribute__ ((visibility ("hidden")));
extern "C" ssa_gpr_t ssa_gpr_dump  __attribute__ ((visibility ("hidden")));
extern "C" void dump_ssa(uint64_t ptcs) __attribute__ ((visibility("hidden")));

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif


extern std::map<uint64_t, std::atomic<char>> sig_flag_map;

void dump_sigaction(void);

typedef struct _ssa_t {

} __attribute__((packed)) ssa_t;
#endif
