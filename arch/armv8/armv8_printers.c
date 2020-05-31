#include <common.h>
#include <bits_api.h>
#include <arch/armv8/cpsr.h>

void print_cpu_flags()
{
  uint64_t nzcv, daif, current_el, sp_sel;
  // pan, uao, dit, ssbs;

  asm volatile(
    "mrs %0, nzcv\n"
    "mrs %1, daif\n"
    "mrs %2, currentel\n"
    "mrs %3, spsel\n" : "=r"(nzcv), "=r"(daif), "=r"(current_el), "=r"(sp_sel));

  printf("cpsr: nzcv: %x, daif: %x(%s%s%s%s), current_el: %x, sp_sel: %x" __endline, 
    nzcv, daif, 
    daif & BT(CPSR_F) ? "no-FIQ" : "",
    daif & BT(CPSR_I) ? "no-IRQ" : "",
    daif & BT(CPSR_A) ? "no-SError" : "",
    daif & BT(CPSR_D) ? "no-DBG" : "",
    current_el >> CPSR_EL_OFF, sp_sel);
}
