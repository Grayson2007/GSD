#ifndef asmx64_h
#define asmx64_h
#include "gsd-common.h"

u64 _rdrand();
u64 _rdseed();
#define _CPUID_EAX 0
#define _CPUID_EBX 1
#define _CPUID_ECX 2
#define _CPUID_EDX 3
#define _CPUID_ARR 
void _cpuid(u32 ecx,u32* out);
u8 inb(u16 port);
u16  inw(u16 port);
u16 ind(u16 port);
void outb(u16 port,u8 val);
void outw(u16 port,u16 val);
void outd(u16 port,u32 val);
u64 _rdmsr(u32 ecx);
void _wrmsr(u32 ecx,u64 val);
void _wbinvd();
void _cli();
void _sti();
void _invlpg(uintptr_t vaddr);


#endif