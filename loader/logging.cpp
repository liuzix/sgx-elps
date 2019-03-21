#include "logging.h"
#include <string>

using namespace std;

std::shared_ptr<spdlog::logger> getLogger(const string &name, spdlog::level::level_enum level) {
    auto logger = spdlog::get(name);
    if (!logger) {
        logger = spdlog::stdout_color_mt(name);
        logger->set_level(level);
    }
    return logger;
}
//std::shared_ptr<spdlog::logger> console = spdlog::stdout_color_mt("loader");
