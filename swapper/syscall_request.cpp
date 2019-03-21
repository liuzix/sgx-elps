#include "syscall_request.h"
#include <logging.h>

std::shared_ptr<spdlog::logger> syscallConsole = spdlog::stdout_color_mt("syscall");

void syscallRequestHandler(SyscallRequest *req) {
    syscallConsole->info("SYSCALL{}", req->fm_list.syscall_num);
}
