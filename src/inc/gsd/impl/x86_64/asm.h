#ifndef asmx64_h
#define asmx64_h
#include <gsd-common.h>
#include <gsd-thread.h>
#include <immintrin.h>

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
uintptr_t _rdfsbase();
void _wrfsbase(uintptr_t addr);
bool _asm_spin_aquire(mutex* lock,size attempt);
void _asm_spin_aquire_ind(mutex* lock);
void _asm_spin_release(mutex* lock);
bool _asm_cmpxchg(gsd_atomic* ptr,int val,int swap);

void _asm_atomic_inc(u32* value);
void _asm_atomic_incl(u64* value);
void _asm_atomic_dec(u32* value);
void _asm_atomic_decl(u64* value);

void _asm_spinlock_aquire(spinlock* lock);
void _asm_spinlock_release(spinlock* lock);
NORETURN void _asm_flush_stack(uintptr_t rsp,uintptr_t rbp,uintptr_t rip);

#endif