section .text
global _asm_disable_irq
_asm_disable_irq:
        cli
        ret
global _asm_enable_irq
_asm_enable_irq:
        sti
        ret
global _asm_atomic_increment
_asm_atomic_increment:
        lock inc dword [rdi]
        ret
global _asm_atomic_decrement
_asm_atomic_decrement:
        lock dec dword [rdi]
        ret

global _asm_atomic_increment_long
_asm_atomic_increment_long:
        lock inc qword [rdi]
        ret
global _asm_atomic_decrement_long
_asm_atomic_decrement_long:
        lock dec qword [rdi]
        ret



global _asm_atomic_bit_test_set
_asm_atomic_bit_test_set:

global _asm_atomic_bit_test_set_long
_asm_atomic_bit_test_set_long:



