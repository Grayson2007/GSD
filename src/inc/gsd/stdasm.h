#ifndef stdasm_h
#define stdasm_h
#include <gsdstd/stdmem.h>

void _asm_disable_irq();
void _asm_enable_irq();

void _asm_atomic_increment(int* x);
void _asm_atomic_decrement(int* x);
void _asm_atomic_increment_long(long* x);
void _asm_atomic_decrement_long(long* x);


void _asm_atomic_bit_test_set(int* x);




#endif