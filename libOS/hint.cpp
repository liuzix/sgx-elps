#include "panic.h"
#include "hint.h"
#include <immintri.h>

extern "C" int __async_swap(void *addr);
void hint(void *addr) {

    unsigned status;
    if ((status = _xbegin()) == _XBEGIN_STARTED) {
        LIBOS_UNUSED char t = *addr;
        _xend ();
    } else
        __async_swap(addr);
}
