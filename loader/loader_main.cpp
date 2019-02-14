#include <iostream>
#include <elfio/elfio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

auto console = spdlog::stdout_color_mt("console");

int main(int argc, char **argv)
{
    console->info("Welcome to the Loader");
    return 0;
}

