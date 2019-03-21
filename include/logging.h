#ifndef LOGGING_H
#define LOGGING_H

//#ifndef IS_LIBOS

#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace std;

//extern std::shared_ptr<spdlog::logger> console;
shared_ptr<spdlog::logger> getLogger(const string &name, spdlog::level::level_enum);

#define DEFINE_LOGGER(name, dbg_level) \
    static std::shared_ptr<spdlog::logger> console               \
              = spdlog::stdout_color_mt(#name);                 \
    struct Logger_##name {                                         \
        Logger_##name (void) {                         \
            console->set_level(dbg_level);                       \
            console->trace("console initialized");               \
        }                                                        \
    } logger_##name;                                                 


#define DEFINE_MEMBER_LOGGER(name, dbg_level) \
    shared_ptr<spdlog::logger> classLogger = getLogger(name, dbg_level);      

//#endif
#endif
