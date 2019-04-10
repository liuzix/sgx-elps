#ifndef PANIC_H
#define PANIC_H

#include <streambuf>
#include <control_struct.h>
#include "thread_local.h"
#include <libOS_tls.h>

#define PANIC_BUFFER_SIZE 256

struct panic_struct;
extern panic_struct *panicInfo;

extern "C" void libos_panic(const char *msg);
void writeToConsole(const char *m);

void initPanic(panic_struct *);

template <typename... Types>
inline void libos_print(const char *format, Types... args) {
    char buf[PANIC_BUFFER_SIZE];
    snprintf(buf, PANIC_BUFFER_SIZE, format, args...);
    writeToConsole(buf);
}

template <typename... Types>
inline void libos_printb(const char *fmt, Types... args) {
    libOS_shared_tls *libOS_data = getSharedTLS();

    char *buf = libOS_data->buffer;
    int i = libOS_data->buffer_index;

    if (i >= PRINT_BUFFER_SIZE - 1) {
        libos_print("libos_printb buffer full. Will use libos_print instead.\n");
        libos_print(fmt, args...);
        return ;
    }
    buf += i;
    /*
     * Currently we do not make it a ring buffer because it seems
     * the buffer size is enough for our debug purpose
     */
    i += snprintf(buf, PRINT_BUFFER_SIZE - i, fmt, args...);
    /*
     * Note: Here we do not perform the i++ so the \0 will be
     * 1) over write by the next printb, or
     * 2) marks the end of the buffer string at the end of the world
     */
    libOS_data->buffer_index = i;
}

#define profile_func(func, ...) \
    do {                                \
        extern uint64_t *pjiffies;      \
        uint64_t jiffies = *pjiffes;    \
        func(__VA_ARGS__);              \
        libos_printb("%s use CPU cycles: %ld\n". #func, *pjiffies - jiffies); \
    }while(0)

template <>
inline void libos_print<>(const char *format) {
    writeToConsole(format);
}

#endif
