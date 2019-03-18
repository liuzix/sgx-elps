#include "debug_request.h"
#include <logging.h>

std::shared_ptr<spdlog::logger> debugConsole = spdlog::stdout_color_mt("enclave");

void debugRequestHandler(DebugRequest *req) {
    debugConsole->info("{}", req->printBuf);      
}
