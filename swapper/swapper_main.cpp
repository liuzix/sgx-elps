#include <thread>
#include <iostream>
#include <functional>
#include <logging.h>
#include <swapper.h>
#include "debug_request.h"
#include "syscall_request.h"
#include "swap_request.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <atomic>
#include <sched.h>
#include "sleep.h"

#define MAX_EVENTS 10

using namespace std;
#pragma GCC visibility push(hidden)
std::shared_ptr<spdlog::logger> console = spdlog::stdout_color_mt("swapper");
#pragma GCC visibility pop
SwapperManager swapperManager;

extern uint64_t __jiffies;

void schedulerRequestHandler(SwapperManager *manager, SchedulerRequest *req);


/** 
 * Set a file descriptor to blocking or non-blocking mode.
 *
 * @param fd The file descriptor
 * @param blocking 0:non-blocking mode, 1:blocking mode
 *
 * @return 1:success, 0:failure.
 **/
int fd_set_blocking(int fd, int blocking) {
    /* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}

char debuggerThreadBuf[DBBUF_SIZE] = {'\0'};
int buf_index = 0;

void debuggerThread() {
    int p = 0;
    while (1) {
        if (debuggerThreadBuf[p] != '\0') {
            cout << debuggerThreadBuf[p] << flush;
            debuggerThreadBuf[p] = '\0';
            p = (p + 1) % DBBUF_SIZE;
        }
        usleep(1);
    }
}

void SwapperManager::runWorker(int id) {
    console->info("Swapper thread started, id = {}", id); 

    sched_param sp;
    sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_setscheduler(0, SCHED_FIFO, &sp);
    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(4 + id * 2, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    RequestDispatcher dispatcher(id);
    dispatcher.addHandler<DebugRequest>(debugRequestHandler);
    dispatcher.addHandler<SyscallRequest>(std::bind(syscallRequestHandler, this, std::placeholders::_1));
    dispatcher.addHandler<SwapRequest>(std::bind(swapRequestHandler, this, std::placeholders::_1));
    dispatcher.addHandler<SchedulerRequest>(std::bind(schedulerRequestHandler, this, std::placeholders::_1));
    dispatcher.addHandler<SleepRequest>(sleepRequestHandler);
//    struct epoll_event ev, events[MAX_EVENTS];
//    int nfds, epollfs;

    /*epollfd = epoll_create1(0);
    if (epollfd == -1) {
        console->error("epoll_create1");
        return;
    }
    */
    //if (id == 0)
    //    debuggerThread();

    while (true) {
        checkSleep(this);
        //uint64_t jiffies;
        //jiffies = __jiffies;
        RequestBase *request = 0x0;
        if (!this->queue.take(request)) continue;
        //if (request->requestType == 3)
            //console->info("take from queue jiffies: {}", __jiffies - jiffies);
        dispatcher.dispatch(request);
    }
/*
        if (false) {
            int fd;
            //parseRequest();
            if (fd_set_blocking(fd, 0) == 0) {
                console->error("set fd nonblocking");
                return;
            }

            // ONESHOT flag is probably needed
            ev.events = EPOLLIN | EPOLLET;
            ev.events = EPOLLOUT | EPOLLET;
            ev.data.fd = fd;
            ev.data.ptr = request;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
                console->error("epoll_ctl");
                return;
            }
        }

        // set timeout to 0 to return immediately 
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, 0);
        if (nfds == -1) {
            console->error("epoll_wait");
            return;
        }

        for (int i = 0; i < nfds; ++i) {
            handleEvent(events[i]);
            // if ONESHOT flag is used, the following should be deleted
            int tmp_fd = events[i].data.fd;
            if (epoll_ctl(epollfd, EPOLL_CTL_DEL, tmp_fd, &ev) == -1) {
                console->error("epoll_ctl");
                return;
            }
        }
    }
    close(epollfd);
  */  console->info("Swapper thread exits, id = {}", id);
}

void SwapperManager::launchWorkers() {
    for (int i = 0; i < this->nThreads; i++) {
        auto job = bind(&SwapperManager::runWorker, this, i);
        this->threads.emplace_back(job);
    }
}

void SwapperManager::waitWorkers() {
    for (int i = 0; i < this->nThreads; i++)
        this->threads[i].join();
}

