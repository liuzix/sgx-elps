#include <vector>
#include <unordered_map>
#include <syscall_format.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <request.h>
#include <cstring>
#include "panic.h"
#include "allocator.h"

using namespace std;

static unordered_map<unsigned int, vector<unsigned int>>* syscall_table;
static unordered_map<unsigned int, unsigned int>* type_table;

void initSyscallTable() {
    syscall_table = new unordered_map<unsigned int, vector<unsigned int>>();
    type_table = new unordered_map<unsigned int, unsigned int>(); 

    add_type_size(CHAR_PTR, 256);
    add_type_size(EVENT_PTR, sizeof(struct epoll_event));
    add_type_size(INT_PTR, sizeof(int));
    add_type_size(IOVEC_PTR, sizeof(struct iovec));
    add_type_size(IOC_PTR, sizeof(struct winsize));
    add_type_size(SOKADDR_PTR, sizeof(struct sockaddr));

    add_syscall0(SYS_GETGID);
    add_syscall1(SYS_CLOSE, NON_PTR);
    add_syscall1(SYS_EPOLL_CREATE, NON_PTR);
    add_syscall1(SYS_EXIT, NON_PTR);
    add_syscall2(SYS_LISTEN, NON_PTR, NON_PTR);
    add_syscall3(SYS_ACCEPT, NON_PTR, SOKADDR_PTR, INT_PTR);
    add_syscall3(SYS_BIND, NON_PTR, SOKADDR_PTR, NON_PTR);
    add_syscall3(SYS_CONNECT, NON_PTR, SOKADDR_PTR, NON_PTR);
    add_syscall3(SYS_GETSOCKNAME, NON_PTR, SOCKADDR_PTR, INT_PTR);
    add_syscall3(SYS_IOCTL, NON_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_MPROTECT, NON_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_OPEN, CHAR_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_READ, NON_PTR, CHAR_PTR, NON_PTR);
    add_syscall3(SYS_SOCKET, NON_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_WRITE, NON_PTR, CHAR_PTR, NON_PTR);
    add_syscall3(SYS_WRITEV, NON_PTR, IOVEC_PTR, NON_PTR);
    add_syscall4(SYS_EPOLL_CTL, NON_PTR, NON_PTR, NON_PTR, EVENT_PTR);
    add_syscall4(SYS_EPOLL_WAIT, NON_PTR, NON_PTR, NON_PTR, EVENT_PTR);
    add_syscall5(SYS_SETSOCKOPT, NON_PTR, NON_PTR, NON_PTR, CHAR_PTR, NON_PTR);
}

/* not really useful*/
static bool isIOsyscall(unsigned int num) {
    return num == SYS_READ || num == SYS_WRITE || num == SYS_CLOSE || num == SYS_IOCTL;
}

/* populate format struct */
bool interpretSyscall(format_t& fm_l, unsigned int index) {
    format_t fm;

    auto it = syscall_table->find(index);
    if (it == syscall_table->end()) {
        libos_print("[%d] not found", index);
        return false;
    }

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

/* some arguments need to be written back
 * specify write back args here
 */
static bool needWriteBack(unsigned int num, unsigned int index) {
    return (num == SYS_READ && index == 1)
        || (num == SYS_EPOLL_WAIT && index == 1)
        || (num == SYS_GETSOCKNAME && index == 1)
        || (num == SYS_GETSOCKNAME && index == 2)
        || (num == SYS_ACCEPT && index == 1)
        || (num == SYS_ACCEPT && index == 2);
}

/* some syscalls have pointer args instead of size_t indicating the arg size
 * set getSIze rules for arguments here */
static unsigned int getSize(const SyscallRequest* req, unsigned int i) {
    unsigned int res = 0;
    unsigned int syscall_n = req->fm_list.syscall_num;

    switch (syscall_n) {
        case SYS_READ:
        case SYS_WRITE:
        case SYS_BIND:
        case SYS_SETSOCKOPT:
            res = (unsigned int)req->args[i + 1].arg;
            break;
        case SYS_ACCEPT:
        case SYS_GETSOCKNAME:
            if (i == 1)
                res = (unsigned int)*((int*)req->args[i + 1].arg);
            else
                res = sizeof(int);
            break;
        case SYS_EPOLL_WAIT:
        case SYS_WRITEV:
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

/* deep copy helper*/
template<typename obj, typename member, typename length>
void memberCopy(obj* src, obj* des, member tar, length len) {
    des->*tar = unsafeMalloc(src->*len);
    memcpy(des->*tar, src->*tar, src->*len);
}

/* deep copy if necessary
 * set deep copy rules here
 */
void deepCopy(SyscallRequest* req, unsigned int index) {
    unsigned int syscall_n = req->fm_list.syscall_num;

    switch (syscall_n) {
        case SYS_WRITEV:{
            iovec* src = (iovec*)req->args[index].arg;
            iovec* des = (iovec*)req->args[index].data;
            for (int i = 0; i < req->args[index + 1].arg; i++)
                memberCopy(src++, des++, &iovec::iov_base, &iovec::iov_len);
            break;
        }
        default:
            break;
    }
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
            deepCopy(this, i);
            this->args[i].arg = (long)this->args[i].data;
        }
    }

    return true;
}

/* deep clean helper*/
template<typename obj, typename member>
void memberClean(obj* src, member tar) {
    unsafeFree(src->*tar);
}

/* free args that are copied inside 
 * set deep clean rules here
 */
void SyscallRequest::deepClean(int index) {
    if (this->fm_list.types[index] == NON_PTR)
        return;
    unsigned int syscall_n = this->fm_list.syscall_num;

    switch (syscall_n) {
        case SYS_WRITEV:{
            iovec* src = (iovec*)this->args[index].data;
            for (int i = 0; i < this->args[index + 1].arg; i++)
                memberClean(src++, &iovec::iov_base);
            break;
                        }
        default:
            break;
    }
}

/* write back helper
 * set write back rules here
 */
void writeBack(SyscallRequest* req, long* enclave_args, unsigned int index) {
    if (req->fm_list.types[index] == NON_PTR) {
        enclave_args[index] = req->args[index].arg;
        return;
    }
    unsigned int syscall_n = req->fm_list.syscall_num;

    switch (syscall_n) {
        case SYS_ACCEPT:
        case SYS_GETSOCKNAME: {
            unsigned int size = req->args[index + 1].arg;
            memcpy((void*)enclave_args[index], (void*)req->args[index].arg, size);
            break;
                              }
        default:
            memcpy((void*)enclave_args[index], (void*)req->args[index].arg,
                   req->fm_list.sizes[index]);
            break;
    }
}

/* copy back arg to enclave memory if needed */
void SyscallRequest::fillEnclave(long* enclave_args) {
     for (unsigned int i = 0; i < this->fm_list.args_num; i++) {
        if (needWriteBack(this->fm_list.syscall_num, i))
            writeBack(this, enclave_args, i);
     }
}
