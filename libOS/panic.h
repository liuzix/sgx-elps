#ifndef PANIC_H
#define PANIC_H
#include <streambuf>
#include <control_struct.h>
#define PANIC_BUFFER_SIZE 256

struct panic_struct;
extern panic_struct *panicInfo;

extern "C" void libos_panic(const char *msg);
void writeToConsole(const char *m);

void initPanic(panic_struct *);

template <typename... Types>
inline void libos_print(const char *format, Types... args) {
    char buf[256];
    sprintf(buf, format, args...);
    writeToConsole(buf);
}

template <>
inline void libos_print<>(const char *format) {
    writeToConsole(format);
}

#endif
