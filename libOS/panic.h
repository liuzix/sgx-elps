#ifndef PANIC_H
#define PANIC_H

#define PANIC_BUFFER_SIZE 256
extern char *panicBuffer;
extern char *requestBuf;

extern "C" void libos_panic(const char *msg);

#endif
