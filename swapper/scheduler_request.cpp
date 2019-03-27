#include "swapper_interface.h"
#include <logging.h>
#include "request.h"

DEFINE_LOGGER(SchedulerRequests, spdlog::level::trace);

void schedulerRequestHandler(SwapperManager *manager, SchedulerRequest *req) {
    if (req->subType == SchedulerRequest::SchedulerRequestType::NewThread) {
        console->info("New Thread reqeust");
        manager->wakeUpThread();
    }
    else if (req->subType == SchedulerRequest::SchedulerRequestType::SchedReady) {
        console->info("SchedReady!");
        manager->schedReady();
    }

}
