#include <time.h>
#include "swapper.h"
#include "sleep.h"
#include <map>

using namespace std;

static std::multimap<unsigned long, RequestBase *> sleepMap;
static struct timespec timeSpec;

void checkSleep(SwapperManager *manager) {
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    unsigned long ns = timeSpec.tv_sec * 1000000000 + timeSpec.tv_nsec;
    
    auto it = sleepMap.begin();
    while (it != sleepMap.end() && it->first < ns) {
        it->second->setDone();
        manager->wakeUpThread(); 
        it = sleepMap.erase(it); 
    }
}

void sleepRequestHandler(SleepRequest *req) {
    unsigned long target = req->ns + 
        timeSpec.tv_sec * 1000000000 + timeSpec.tv_nsec;
    sleepMap.insert(std::make_pair(target, req));
}
