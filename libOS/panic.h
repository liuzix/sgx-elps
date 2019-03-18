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
std::streambuf *getPanicStreamBuf();

#endif
