#ifndef SCHED_H
#define SCHED_H
#include "../loader/enclave_thread.h"
#include <boost/intrusive/list.hpp>

#define MAXIMUM_SLOT 20

using namespace boost::intrusive;

class SchedEntity {
public:
    SchedEntity();
    int timeSlot;
    void switchTo();
    list_member_hook<> member_hook_;
};

typedef list< SchedEntity
            , member_hook< SchedEntity, list_member_hook<>, &SchedEntity::member_hook_>
            > SchedQueue;

extern SchedQueue schedQueue;







#endif
