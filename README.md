# SGX Exitless Page Swapping Project

### how to build
*Building does not require an SGX enabled computer, so you can check if it compiles even if you don't have an SGX capable machine.*

Preparation (assuming Ubuntu 18.04):

> sudo apt install cmake libssl-dev libprotobuf-dev 

Generate Makefile:
> cd loader

> cmake .

Build it:
> make

The samples should also be built:
> cd samples
> make

### how to test
*Testing requires an SGX enabled machine with intel's SGX driver and SGX psw installed*
> cd loader
> ./loader ../samples/hello

