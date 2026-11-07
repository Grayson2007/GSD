#include <gsd-common.h>
#include <gsd-memory.h>
#include <gsd-thread.h>
#define ATTRIB_ALLOCATED 1 

static inline int _log2ceil(size x) {
        int y = 0;
        size l = 1;
        while(l < y) {
                y++;
                l <<= 1;
        }
        return y;
}


status 