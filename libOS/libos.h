#ifndef LIBOS_H
#define LIBOS_H
#include <request.h>
#include <queue.h>

extern Queue<RequestBase> *requestQueue;
extern "C" int main(int argc, char **argv);
#endif
