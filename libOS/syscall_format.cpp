#include <syscall_format.h>
#include <request.h>
#include <vector>
#include <unordered_map>
#include <cstring>
#include "allocator.h"
#include <sys/epoll.h>

using namespace std;

static unordered_map<unsigned int, vector<unsigned int>>* syscall_table;
static unordered_map<unsigned int, unsigned int>* type_table;

void initSyscallTable() {
    syscall_table = new unordered_map<unsigned int, vector<unsigned int>>();
    type_table = new unordered_map<unsigned int, unsigned int>(); 

    add_type_size(CHAR_PTR, 256);
    add_type_size(EVENT_PTR, sizeof(struct epoll_event));
    add_type_size(INT_PTR, sizeof(int));

    add_syscall3(SYS_READ, NON_PTR, CHAR_PTR, NON_PTR);
    add_syscall3(SYS_WRITE, NON_PTR, CHAR_PTR, NON_PTR);
    add_syscall3(SYS_OPEN, CHAR_PTR, NON_PTR, NON_PTR);
    add_syscall1(SYS_CLOSE, NON_PTR);
    add_syscall1(SYS_EXIT, NON_PTR);
    add_syscall3(SYS_IOCTL, NON_PTR, NON_PTR, NON_PTR);
    add_syscall0(SYS_GETGID);
    add_syscall3(SYS_MPROTECT, NON_PTR, NON_PTR, NON_PTR);
    add_syscall1(SYS_EPOLL_CREATE, NON_PTR);
    add_syscall4(SYS_EPOLL_CTL, NON_PTR, NON_PTR, NON_PTR, EVENT_PTR);
    add_syscall3(SYS_GETSOCKNAME, NON_PTR, SOCKADDR_PTR, INT_PTR);
}

static bool isIOsyscall(unsigned int num) {
    return num == SYS_READ || num == SYS_WRITE || num == SYS_CLOSE || num == SYS_IOCTL;
}

bool interpretSyscall(format_t& fm_l, unsigned int index) {
    format_t fm;

    auto it = syscall_table->find(index);
    if (it == syscall_table->end())
        return false;

    if (isIOsyscall(index))
        fm.isIO = false;
    else
        fm.isIO = true;
    fm.syscall_num = index;
    fm.args_num = it->second.size();
    for (unsigned int i = 0; i < fm.args_num; i++) {
        fm.types[i] = it->second[i];
        fm.sizes[i] = 0;
    }
    fm_l = fm;
    return true;
}

/*
static bool sizeInArgs(unsigned int num) {
    return num == SYS_READ || num == SYS_WRITE
        || num == SYS_GETSOCKNAME;
}
*/
static bool needWriteBack(unsigned int num, unsigned int index) {
    return (num == SYS_READ && index == 1)
        || (num == SYS_EPOLL_WAIT && index == 1)
        || (num == SYS_GETSOCKNAME && index == 1);
}
/*
static unsigned int sizeFactor(const class SyscallRequest* req) {
    unsigned int factor = 1;
    if (req->fm_list.syscall_num == SYS_EPOLL_WAIT)
        factor = (unsigned int)req->args[2].arg;
    return factor;
}
*/

/* some syscalls have pointer args instead of size_t indicating the arg size */
static unsigned int getSize(const SyscallRequest* req, unsigned int i) {
    unsigned int res = 0;
    unsigned int syscall_n = req->fm_list.syscall_num;

    switch (syscall_n) {
        case SYS_READ: 
        case SYS_WRITE:
            res = (unsigned int)req->args[i + 1].arg;
            break;
        case SYS_GETSOCKNAME:
            if (i == 1)
                res = (unsigned int)*((int*)req->args[i + 1].arg);
            else
                res = sizeof(int);
            break;
        case SYS_EPOLL_WAIT:
            if (type_table->count(req->fm_list.types[i]) == 0)
                return 0;
            res = type_table->at(req->fm_list.types[i])
                    * (unsigned int)req->args[i + 1].arg;
            break;
        default:
            if (type_table->count(req->fm_list.types[i]) == 0)
                return 0;
            res = type_table->at(req->fm_list.types[i]);
            break;
    }
    return res;
}

void deepcopy(SyscallRequest* req, unsigned int i) {

}

/* copy arg to unsafe memory if needed */
bool SyscallRequest::fillArgs() {
    for (unsigned int i = 0; i < this->fm_list.args_num; i++) {
        if (this->fm_list.types[i] == NON_PTR)
            continue;
        else {
            unsigned int arg_size;
            arg_size = getSize(this, i);    
            if (arg_size == 0)
                return false;
            this->fm_list.sizes[i] = arg_size;
            this->args[i].data = (char*)unsafeMalloc(arg_size);
            memcpy(this->args[i].data, (void*)this->args[i].arg, arg_size);
            deepcopy(this, i);
            this->args[i].arg = (long)this->args[i].data;
        }
    }

    return true;
}

/* copy back arg to enclave memory if needed */
void SyscallRequest::fillEnclave(long* enclave_args) {
     for (unsigned int i = 0; i < this->fm_list.args_num; i++) {
        if (needWriteBack(this->fm_list.syscall_num, i))
            memcpy((void*)enclave_args[i], (void*)this->args[i].arg, this->fm_list.sizes[i]);
     }
}
