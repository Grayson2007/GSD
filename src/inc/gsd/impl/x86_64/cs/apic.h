#ifndef apic_h
#define apic_h
#include <gsd-common.h>

void write_ioapic(void* ioapic_addr,u8 reg,u32 value);
u32 read_ioapic(void* ioapic_addr,u8 reg);





#endif