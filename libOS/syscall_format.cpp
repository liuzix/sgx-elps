#include <syscall_format.h>
#include <request.h>
#include <vector>
#include <unordered_map>
#include <cstring>
#include "allocator.h"

using namespace std;

/* support syscall: 0 1 2 3  */
static unordered_map<unsigned int, vector<unsigned int>>* syscall_table;
static unordered_map<unsigned int, unsigned int>* type_table;

void initSyscallTable() {
    syscall_table = new unordered_map<unsigned int, vector<unsigned int>>();
    type_table = new unordered_map<unsigned int, unsigned int>(); 

    type_table->emplace(CHAR_PTR, 256);
    syscall_table->emplace(SYS_READ, vector<unsigned int>({ NON_PTR, CHAR_PTR, NON_PTR }));
    syscall_table->emplace(SYS_WRITE, vector<unsigned int>({ NON_PTR, CHAR_PTR, NON_PTR }));
    syscall_table->emplace(SYS_OPEN, vector<unsigned int>({ CHAR_PTR, NON_PTR, NON_PTR }));
    syscall_table->emplace(SYS_CLOSE, vector<unsigned int>({ NON_PTR }));
    syscall_table->emplace(SYS_EXIT, vector<unsigned int>({ NON_PTR }));
    syscall_table->emplace(SYS_IOCTL, vector<unsigned int>({ NON_PTR, NON_PTR, NON_PTR }));
}


bool interpretSyscall(format_t& fm_l, unsigned int index) {
    format_t fm;

    auto it = syscall_table->find(index);
    if (it == syscall_table->end())
        return false;

    fm.syscall_num = index;
    fm.args_num = it->second.size();
    for (unsigned int i = 0; i < fm.args_num; i++) {
        fm.types[i] = it->second[i];
        fm.sizes[i] = 0;
    }
    fm_l = fm;
    return true;
}

/* syscall that has given size of ptr area
 * size is given just after ptr argument
 */
static bool noSizeNeed(unsigned int num) {
    return num == SYS_READ || num == SYS_WRITE;
}

/* syscall that needs to write back to enclave */
static bool needWriteBack(unsigned int num, unsigned int index) {
    return (num == SYS_READ && index == 1);
}

bool SyscallRequest::fillArgs() {
    for (unsigned int i = 0; i < this->fm_list.args_num; i++) {
        if (this->fm_list.types[i] == NON_PTR)
            continue;
        else if (noSizeNeed(this->fm_list.syscall_num)) {
            unsigned int arg_size;
            arg_size = (unsigned int)this->args[i + 1].arg;
            this->fm_list.sizes[i] = arg_size;
            this->args[i].data = (char*)unsafeMalloc(arg_size);
            memcpy(this->args[i].data, (void*)this->args[i].arg, arg_size);
            this->args[i].arg = (long)this->args[i].data;
        } else {
            unsigned int arg_size;
            if (type_table->count(this->fm_list.types[i]) == 0)
                return false;
            arg_size = type_table->at(this->fm_list.types[i]);
            this->fm_list.sizes[i] = arg_size;
             this->args[i].data = (char*)unsafeMalloc(arg_size);
            memcpy(this->args[i].data, (void*)this->args[i].arg, arg_size);
            this->args[i].arg = (long)this->args[i].data;
            if (this->fm_list.types[i] == CHAR_PTR)
                *(this->args[i].data + arg_size - 1) = '\0';
        }
    }

    return true;
}

void SyscallRequest::fillEnclave(long* enclave_args) {
     for (unsigned int i = 0; i < this->fm_list.args_num; i++) {
        if (needWriteBack(this->fm_list.syscall_num, i))
            memcpy((void*)enclave_args[i], (void*)this->args[i].arg, this->fm_list.sizes[i]);
     }
}
