#pragma once

void wait_cycles(unsigned int n);
void wait_msec(unsigned int n);
unsigned long long get_system_timer();
void wait_msec_st(unsigned int n);

void set_timer(unsigned int ms);
