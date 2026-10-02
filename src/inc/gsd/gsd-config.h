#ifndef gsd_config_h
#define gsd_config_h

// Generic Configuration macros 
#define GSD_KERNEL_RELEASE_NAME "Chair"

#define GSD_KERNEL_RELEASE_VERSION "0.1"



// Architecture Specific Configuration
#define GSD_ARCHITECTURE x86_64
#define ARCH_CONFIGURATION <impl/GSD_ARCHITECTURE/gsd-archcfg.h>
#include ARCH_CONFIGURATION

#endif