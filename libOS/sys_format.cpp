#include "sys_format.h"
#include "allocator.h"
#include "request.h"

#include <vector>
#include <unordered_map>
#include <cstring>
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

static bool sizeInArgs(const unsigned int& num) {
    return num == 0 || num == 1;
}

bool SyscallRequest::fillArgs() {
    for (unsigned int i = 0; i < this->fm_list.args_num; i++) {
        if (this->fm_list.types[i] == NON_PTR)
            continue;
        else if (sizeInArgs(this->fm_list.syscall_num)) {
            unsigned int arg_size;
            arg_size = (unsigned int)this->args[i + 1].arg;
            this->fm_list.sizes[i] = arg_size;
            this->args[i].data = new char[arg_size];
            memcpy(this->args[i].data, (void*)this->args[i].arg, arg_size);
        } else {
            unsigned int arg_size;
            if (type_table->count(this->fm_list.types[i]) == 0)
                return false;
            arg_size = type_table->at(this->fm_list.types[i]);
            this->fm_list.sizes[i] = arg_size;
            this->args[i].data = new char[arg_size];
            memcpy(this->args[i].data, (void*)this->args[i].arg, arg_size);
            if (this->fm_list.types[i] == CHAR_PTR)
                *(this->args[i].data + arg_size - 1) = '\0';
        }
    }

    return true;
}

bool createArg(long val, int i, format_t& fm_l, syscall_arg_t& arg_t) {
    return true;
}
