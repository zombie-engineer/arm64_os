#pragma once

typedef int(*irq_cb_t)();

void set_irq_cb(irq_cb_t cb);

void set_fiq_cb(irq_cb_t cb);

void generate_exception();

void irq_handle_generic();
