#include <string>
#include <iostream>
#include <elfio/elfio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "enclave_manager.h"

using namespace std;
using namespace ELFIO;

auto console = spdlog::stdout_color_mt("console");

void loadMain(const string &filename)
{
    elfio reader;

    if (!reader.load(filename)) {
        console->error("Unable to load {}", filename);
        exit(-1);
    }

    if (reader.get_class() != ELFCLASS64
            || reader.get_machine() != EM_X86_64) {
        console->error("Unsupported architecture");
        exit(-1);
    }

    EnclaveManager enclaveManager((void *)0x10000000, 0x8000);
}



int main(int argc, char **argv)
{
    if (argc < 2) {
        console->error("Usage: loader [binary file name]");
        return -1;
    }


    console->info("Welcome to the Loader");
    console->info("Start loading binary file: {}", argv[1]);

    loadMain(argv[1]);
    return 0;
}

