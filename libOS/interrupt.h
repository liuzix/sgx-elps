#pragma once

extern "C" void __interrupt_exit();
extern "C" void do_interrupt(void *tcs);
extern "C" void do_preempt();
