#include "swap_request.h"
#include <logging.h>
#include <sys/ioctl.h>
#include <fcntl.h>

std::shared_ptr<spdlog::logger> swapConsole = spdlog::stdout_color_mt("swap");

static int deviceHandle() {
    static int fd = -1;
    if (fd >= 0)
        return fd;
    fd = open("/dev/isgx", O_RDWR);
    if (fd < 0) {
        swapConsole->error("Opening /dev/isgx failed: {}.", strerror(errno));
        swapConsole->info("Make sure you have the Intel SGX driver installed.");
        exit(-1);
    }

    return fd;
}

void swapRequestHandler(SwapRequest *req) {
    struct sgx_enclave_swap_page sswap;
    sswap.addr = req->addr;
    int res = ioctl(deviceHandle(), SGX_IOC_ENCLAVE_SWAP_PAGE, &sswap);
    swapConsole->info("Return value from ioctl swap page: {}", res);
    swapConsole->info("Address when calling to swap: 0x{:x}", req->addr);
    swapConsole->info("swap arg addresss: 0x{:x}", (uint64_t)&sswap);
}
