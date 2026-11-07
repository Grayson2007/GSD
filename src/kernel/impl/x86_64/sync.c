#include <gsd-thread.h>
#include <impl/x86_64/asm.h>
void spinlock_aquire(spinlock* lock) {
        return _asm_spinlock_aquire(lock);
}
void spinlock_release(spinlock* lock) {
        return _asm_spinlock_release(lock);
}
