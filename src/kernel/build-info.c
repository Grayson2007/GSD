#include "gsd-common.h"
#include "gsd-memory.h"
#define BUILDPARAM(a) __attribute__((section(".kbininfo"))) a
// Embed Binary Info for the kernel here 
// 
BUILDPARAM(const int OPT_MEMORY_ORDER = GSD_MEM_ORDER;)
