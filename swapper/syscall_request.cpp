#include "syscall_request.h"
#include <logging.h>
#include <sys/syscall.h>


std::shared_ptr<spdlog::logger> syscallConsole = spdlog::stdout_color_mt("syscall");

void syscallRequestHandler(SyscallRequest *req) {
    

    syscallConsole->info("-----SYSCALL({})-----", req->fm_list.syscall_num);
    for (int i = 0; i < 6; i++) {
        syscallConsole->info("arg[{}] arg_content[{:x}]", i+1, req->args[i].arg);
        if (req->fm_list.sizes[i])
            syscallConsole->info("buf: {}", req->args[i].data);
    }

    req->sys_ret = (long)syscall(req->fm_list.syscall_num,
                                          req->args[0].arg,
                                          req->args[1].arg,
                                          req->args[2].arg,
                                          req->args[3].arg,
                                          req->args[4].arg,
                                          req->args[5].arg);
                                           
}
