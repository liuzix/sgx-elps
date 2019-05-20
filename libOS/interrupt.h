#pragma once

extern "C" void __interrupt_exit();
extern "C" void do_interrupt(void *tcs);
extern "C" volatile void do_preempt();
