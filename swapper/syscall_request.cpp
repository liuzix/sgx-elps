#include <logging.h>
#include <sys/syscall.h>
#include <boost/bind.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio.hpp>
#include "syscall_request.h"
#include <syscall_format.h>

#define EP_POOL_SIZE 10

std::shared_ptr<spdlog::logger> syscallConsole = spdlog::stdout_color_mt("syscall");

boost::asio::thread_pool ep_pool(EP_POOL_SIZE);

std::atomic_int epollCount;

void __epollRequestHandler(SwapperManager *manager, SyscallRequest *req) {
    //syscallConsole->info("start processing epoll");
    req->sys_ret = (long)syscall(req->fm_list.syscall_num,
                                 req->args[0].arg,
                                 req->args[1].arg,
                                 req->args[2].arg,
                                 req->args[3].arg,
                                 req->args[4].arg,
                                 req->args[5].arg);
    //syscallConsole->info("end processing epoll");
    if (req->sys_ret == -1) {
        //syscallConsole->info("syscall failed, reading errno");
        req->sys_ret = -errno;
    }

    //syscallConsole->info("finish syscall({})", req->fm_list.syscall_num);
    req->setDone();
    epollCount--;
    //syscallConsole->info("end epoll_wait, now pending {}", epollCount.load());
    manager->wakeUpThread();

}

void syscallRequestHandler(SwapperManager *manager, SyscallRequest *req) {

    
    //syscallConsole->info("-----SYSCALL({})-----", req->fm_list.syscall_num);
    /*syscallConsole->info("syscall request at 0x{:x}", (uint64_t)req);


    for (int i = 0; i < 6; i++) {
        syscallConsole->info("arg[{}] arg_content[{:x}]", i+1, req->args[i].arg);
        if (req->fm_list.sizes[i])
            syscallConsole->info("buf: {}", (char*)req->args[i].arg);
    }
    */

    if (req->fm_list.syscall_num == SYS_EPOLL_WAIT ||
        req->fm_list.syscall_num == SYS_EPOLL_PWAIT ) {
        epollCount++;
        //syscallConsole->info("new epoll_wait, now pending {}", epollCount.load());
        assert(epollCount <= 10);
        boost::asio::post(ep_pool, std::bind(__epollRequestHandler, manager, req));
        return ;
    }
    req->sys_ret = (long)syscall(req->fm_list.syscall_num,
                                          req->args[0].arg,
                                          req->args[1].arg,
                                          req->args[2].arg,
                                          req->args[3].arg,
                                          req->args[4].arg,
                                          req->args[5].arg);
    if (req->sys_ret == -1) {
        syscallConsole->info("syscall[{}] failed, errno = {}", req->fm_list.syscall_num, errno);
        req->sys_ret = -errno;
        /*
        for (int i = 0; i < 6; i++) {
            syscallConsole->info("arg[{}] arg_content[{:x}]", i+1, req->args[i].arg);
            if (req->fm_list.sizes[i])
                syscallConsole->info("buf: {}", (char*)req->args[i].arg);
        }*/
    }

    //syscallConsole->info("----prefinish sysycall({})-----", req->fm_list.syscall_num);
    req->setDone();
    //syscallConsole->info("-----finish syscall({})-----", req->fm_list.syscall_num);
    manager->wakeUpThread();
}

