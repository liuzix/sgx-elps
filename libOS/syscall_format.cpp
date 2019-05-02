#include <vector>
#include <unordered_map>
#include <syscall_format.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <time.h>
#include <sys/utsname.h>
#include <type_traits>
#include <request.h>
#include <cstring>
#include "panic.h"
#include "allocator.h"

using namespace std;

unordered_map<unsigned int, vector<unsigned int>>* syscall_table;
static unordered_map<unsigned int, unsigned int>* type_table;
static SpinLock *table_lock;

void initSyscallTable() {
    if (table_lock) __asm__("ud2");
    table_lock = new SpinLock;
    syscall_table = new unordered_map<unsigned int, vector<unsigned int>>();
    type_table = new unordered_map<unsigned int, unsigned int>();

    add_type_size(CHAR_PTR, 256);
    add_type_size(EVENT_PTR, sizeof(struct epoll_event));
    add_type_size(INT_PTR, sizeof(int));
    add_type_size(IOVEC_PTR, sizeof(struct iovec));
    add_type_size(IOC_PTR, sizeof(struct winsize));
    add_type_size(SOKADDR_PTR, sizeof(struct sockaddr));
    add_type_size(MSGHDR_PTR, sizeof(struct msghdr));
    add_type_size(SIGSET_PTR, sizeof(sigset_t));
    add_type_size(TIMESPEC_PTR, sizeof(struct timespec));
    add_type_size(OLD_UTSNAME_PTR, sizeof(struct old_utsname));
    add_type_size(STAT_PTR, sizeof(struct stat));
    add_type_size(TIMEVAL_PTR, sizeof(struct timeval));
    add_type_size(TIMEZONE_PTR, sizeof(struct timezone));

    add_syscall0(SYS_GETGID);
    add_syscall1(SYS_CLOSE, NON_PTR);
    add_syscall1(SYS_EPOLL_CREATE, NON_PTR);
    add_syscall1(SYS_EPOLL_CREATE1, NON_PTR);
    add_syscall1(SYS_EXIT, NON_PTR);
    add_syscall1(SYS_UNAME, OLD_UTSNAME_PTR);
    add_syscall1(SYS_DUP, NON_PTR);
    add_syscall2(SYS_GETTIMEOFDAY, TIMEVAL_PTR, TIMEZONE_PTR);
    add_syscall2(SYS_LISTEN, NON_PTR, NON_PTR);
    add_syscall2(SYS_CLOCK_GETTIME, NON_PTR, TIMESPEC_PTR);
    add_syscall2(SYS_ACCESS, CHAR_PTR, NON_PTR);
    add_syscall2(SYS_FSTAT, NON_PTR, STAT_PTR);
    add_syscall3(SYS_ACCEPT, NON_PTR, SOKADDR_PTR, INT_PTR);
    add_syscall3(SYS_BIND, NON_PTR, SOKADDR_PTR, NON_PTR);
    add_syscall3(SYS_CONNECT, NON_PTR, SOKADDR_PTR, NON_PTR);
    add_syscall3(SYS_GETSOCKNAME, NON_PTR, SOCKADDR_PTR, INT_PTR);
    add_syscall3(SYS_IOCTL, NON_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_MPROTECT, NON_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_FCNTL, NON_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_OPEN, CHAR_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_READ, NON_PTR, CHAR_PTR, NON_PTR);
    add_syscall3(SYS_SOCKET, NON_PTR, NON_PTR, NON_PTR);
    add_syscall3(SYS_WRITE, NON_PTR, CHAR_PTR, NON_PTR);
    add_syscall3(SYS_WRITEV, NON_PTR, IOVEC_PTR, NON_PTR);
    add_syscall3(SYS_SENDMSG, NON_PTR, MSGHDR_PTR, NON_PTR);
    add_syscall3(SYS_RECVMSG, NON_PTR, MSGHDR_PTR, NON_PTR);
    add_syscall4(SYS_OPENAT, NON_PTR, CHAR_PTR, NON_PTR, NON_PTR);
    add_syscall4(SYS_ACCEPT4, NON_PTR, SOKADDR_PTR, INT_PTR, NON_PTR);
    add_syscall4(SYS_EPOLL_CTL, NON_PTR, NON_PTR, NON_PTR, EVENT_PTR);
    add_syscall4(SYS_EPOLL_WAIT, NON_PTR, EVENT_PTR, NON_PTR, NON_PTR);
    add_syscall5(SYS_SETSOCKOPT, NON_PTR, NON_PTR, NON_PTR, CHAR_PTR, NON_PTR);
    add_syscall6(SYS_SENDTO, NON_PTR, CHAR_PTR, NON_PTR, NON_PTR, SOKADDR_PTR, NON_PTR);
    add_syscall6(SYS_RECVFROM, NON_PTR, CHAR_PTR, NON_PTR, NON_PTR, SOKADDR_PTR, INT_PTR);
    add_syscall6(SYS_EPOLL_PWAIT, NON_PTR, EVENT_PTR, NON_PTR, NON_PTR, SIGSET_PTR, NON_PTR);
}

/* not really useful*/
static bool isIOsyscall(unsigned int num) {
    return num == SYS_READ || num == SYS_WRITE || num == SYS_CLOSE || num == SYS_IOCTL;
}

/* populate format struct */
bool interpretSyscall(format_t& fm_l, unsigned int index) {
    format_t fm;
    table_lock->lock();
    auto it = syscall_table->find(index);
    if (it == syscall_table->end()) {
        libos_print("[%d] not found", index);
        table_lock->unlock();
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
    table_lock->unlock();
    return true;
}

/* some arguments need to be written back
 * specify write back args here
 */
static bool needWriteBack(unsigned int num, unsigned int index) {
    return (num == SYS_READ && index == 1)
        || (num == SYS_EPOLL_WAIT && index == 1)
        || (num == SYS_EPOLL_PWAIT && index == 1)
        || (num == SYS_GETSOCKNAME && index == 1)
        || (num == SYS_GETSOCKNAME && index == 2)
        || (num == SYS_ACCEPT && index == 1)
        || (num == SYS_ACCEPT && index == 2)
        || (num == SYS_RECVFROM && index == 4)
        || (num == SYS_RECVFROM && index == 5)
        || (num == SYS_RECVMSG && index == 1);
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
        case SYS_SENDTO:
        case SYS_CONNECT:
            res = (unsigned int)req->args[i + 1].arg;
            break;
        case SYS_RECVFROM:
            if (i == 1)
                res = (unsigned int)req->args[i + 1].arg;
            else if (i == 4)
                res = (unsigned int)*((int*)req->args[i + 1].arg);
            else
                res = sizeof(int);
            break;
        case SYS_ACCEPT:
        case SYS_GETSOCKNAME:
            if (i == 1)
                res = (unsigned int)*((int*)req->args[i + 1].arg);
            else
                res = sizeof(int);
            break;
        case SYS_EPOLL_PWAIT:
            if (i == 4)
                res = (unsigned int)req->args[i + 1].arg;
            else {
                if (type_table->count(req->fm_list.types[i]) != 1)
                    return 0;
                res = type_table->at(req->fm_list.types[i])
                        * (int)req->args[i + 1].arg;
            }
            break;
        case SYS_EPOLL_WAIT:
        case SYS_WRITEV:
            if (type_table->count(req->fm_list.types[i]) != 1)
                return 0;
            res = type_table->at(req->fm_list.types[i])
                    * (int)req->args[i + 1].arg;
            break;
        default:
            if (type_table->count(req->fm_list.types[i]) != 1)
                return 0;
            res = type_table->at(req->fm_list.types[i]);
            break;
    }
    return res;
}

/* deep copy helper*/
template<typename obj, typename member, typename length>
void memberCopy(obj* src, obj* des, member tar, length len, int fac = 1) {
    using tar_t = decltype(des->*tar);
    using mem_t = typename remove_reference<tar_t>::type;

    int size = (src->*len) * fac;
    libos_print("deepcopy alloca size[%d]", size);
    des->*tar = (mem_t)unsafeMalloc(size);
    memcpy(des->*tar, src->*tar, size);
}

/* deep copy if necessary
 * set deep copy rules here
 */
void deepCopy(SyscallRequest* req, unsigned int index) {
    unsigned int syscall_n = req->fm_list.syscall_num;

    switch (syscall_n) {
        case SYS_WRITEV:{
            /* struct iovec {
             * void  *iov_base;
             * size_t iov_len; 
             * }
             */
            iovec* src = (iovec*)req->args[index].arg;
            iovec* des = (iovec*)req->args[index].data;
            for (int i = 0; i < req->args[index + 1].arg; i++)
                memberCopy(src++, des++, &iovec::iov_base, &iovec::iov_len);
            break;
        }
        case SYS_RECVMSG:
        case SYS_SENDMSG:{
            /* struct msghdr {
             * void         *msg_name;
             * socklen_t     msg_namelen;
             * struct iovec *msg_iov;
             * size_t        msg_iovlen;
             * void         *msg_control;
             * size_t        msg_controllen;
             * int           msg_flags;
             * }
             */
            msghdr* src = (msghdr*)req->args[index].arg;
            msghdr* des = (msghdr*)req->args[index].data;
            memberCopy(src, des, &msghdr::msg_name, &msghdr::msg_namelen);
            memberCopy(src, des, &msghdr::msg_iov, &msghdr::msg_iovlen, sizeof(struct iovec));
            memberCopy(src, des, &msghdr::msg_control, &msghdr::msg_controllen);
            iovec* src_iov = src->msg_iov;
            iovec* des_iov = des->msg_iov;
            for (unsigned int i = 0; i < src->msg_iovlen; i++)
                memberCopy(src_iov++, des_iov++, &iovec::iov_base, &iovec::iov_len);
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
            if (arg_size < 0)
                return false;
            this->fm_list.sizes[i] = arg_size;
            this->args[i].data = (char*)unsafeMalloc(arg_size);
            if (this->args[i].arg)
                memcpy(this->args[i].data, (void*)this->args[i].arg, arg_size);
            libos_print("alloca size[%d]\n", arg_size);
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
            iovec* src = (iovec*)this->args[index].arg;
            for (int i = 0; i < this->args[index + 1].arg; i++)
                memberClean(src++, &iovec::iov_base);
            break;
                        }
        case SYS_RECVMSG:
        case SYS_SENDMSG:{
            msghdr* src = (msghdr*)this->args[index].arg;
            memberClean(src, &msghdr::msg_name);
            memberClean(src, &msghdr::msg_control);
            iovec* src_iov = src->msg_iov;
            for (unsigned int i = 0; i < src->msg_iovlen; i++)
                memberClean(src_iov++, &iovec::iov_base);
            memberClean(src, &msghdr::msg_iov);
            break;
                         }
        default:
            break;
    }
}

/* write back helper2*/
template<typename obj, typename member>
void memberWriteBack(obj* src, obj* des, member tar, int len) {
    memcpy(des->*tar, src->*tar, len);
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
        case SYS_RECVMSG: {
                msghdr* src = (msghdr*)req->args[index].arg;
                msghdr* des = (msghdr*)enclave_args[index];
                memberWriteBack(src, des, &msghdr::msg_name, des->msg_namelen);
                memberWriteBack(src, des, &msghdr::msg_control, des->msg_controllen);
                iovec* src_t = src->msg_iov;
                iovec* des_t = des->msg_iov;
                for (unsigned int i = 0; i < src->msg_iovlen; i++, src_t++, des_t++) {
                    memberWriteBack(src_t, des_t, &iovec::iov_base, des_t->iov_len);
                    des_t->iov_len = src_t->iov_len;
                }
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

extern "C" void print_syscall_table_size() {
}
