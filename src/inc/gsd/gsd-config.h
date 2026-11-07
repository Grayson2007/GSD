#ifndef gsd_config_h
#define gsd_config_h
#include <gsdstd/string.h>
// Assembly config macros
#define USE_ASM_BIG4 0 // set this to '1' to use optimised assembly routines if avalible 
// big 4 referrs to memcpy,memcmp,memset,memmove 
#define USE_ASM_MEMCPY 0
#define USE_ASM_MEMMOVE 0 
#define USE_ASM_MEMSET 0
#define USE_ASM_MEMCMP 0
// Generic Configuration macros 
#define GSD_KERNEL_RELEASE_NAME "Chair"
#define GSD_KERNEL_RELEASE_VERSION "0.1"

#define SEMAPHORE_DEFAULT_ATTEMPS 12
#define MUTEX_DEFAULT_ATTEMPTS 12 
// Architecture Specific Configuration
#ifndef GSD_ARCHITECTURE
#define GSD_ARCHITECTURE x86_64
#endif

#define ARCH_CONFIGURATION M_STRINGIFY(impl/GSD_ARCHITECTURE/gsd-archcfg.h)
#include ARCH_CONFIGURATION 

#endif