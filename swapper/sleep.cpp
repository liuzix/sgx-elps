#include <time.h>
#include <spin_lock.h>
#include "swapper.h"
#include "sleep.h"
#include <map>

using namespace std;

SpinLock sleepLock;
static std::multimap<unsigned long, RequestBase *> sleepMap;
static struct timespec timeSpec;

void checkSleep(SwapperManager *manager) {
    sleepLock.lock();
    
    unsigned long ns = timeSpec.tv_sec * 1000000000 + timeSpec.tv_nsec;
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    auto it = sleepMap.begin();
    while (it != sleepMap.end() && it->first < ns) {
        it->second->setDone();
        manager->wakeUpThread(); 
        it = sleepMap.erase(it); 
    }
    
    sleepLock.unlock();
}

void sleepRequestHandler(SleepRequest *req) {
    sleepLock.lock();

    unsigned long target = req->ns + 
        timeSpec.tv_sec * 1000000000 + timeSpec.tv_nsec;
    sleepMap.insert(std::make_pair(target, req));

    sleepLock.unlock();
}
