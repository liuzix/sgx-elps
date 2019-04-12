#include "swap_request.h"
#include <logging.h>
#include <sys/ioctl.h>
#include <fcntl.h>

std::shared_ptr<spdlog::logger> swapConsole = spdlog::stdout_color_mt("swap");
extern uint64_t __jiffies;

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
    //uint64_t jiffies = __jiffies;
    struct sgx_enclave_swap_page sswap;
    sswap.addr = req->addr;
    //swapConsole->info("hint addr: {}", req->addr);
    ioctl(deviceHandle(), SGX_IOC_ENCLAVE_SWAP_PAGE, &sswap);
    //swapConsole->info("ioctl jiffies: {}", __jiffies - jiffies);
}
