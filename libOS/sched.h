#ifndef SCHED_H
#define SCHED_H
#include "../loader/enclave_thread.h"
#include <boost/intrusive/list.hpp>

#define MAXIMUM_SLOT 20
class SchedEntity {
public:
    SchedEntity(EnclaveThread *eth);
    int timeSlot;
    EnclaveThread ethread;
    list_member_hook<> member_hook_;
};

typedef list< SchedEntity
            , member_hook< SchedEntity, list_member_hook<>, &SchedEntity::member_hook_>
            > SchedQueue;

SchedQueue schedQueue;







#endif
