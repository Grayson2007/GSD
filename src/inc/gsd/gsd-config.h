#ifndef gsd_config_h
#define gsd_config_h

// contains macros for Configuration 
// GSD_MEM_ORDER defines the max order of pages that can be allocated at once in the memory manger 
// Default is 12 which on x86_64 is 16MB blocks 
#define GSD_MEM_ORDER 12
// More Compilation Options will be avalible here 
// These are stored in the .kbininfo section of the output binary 
// If you are loading the kernel via a bootloader you'd access this section to locate the paramaters 
// Relese Name Classifies 
// Releases work as follows
// [Name] [Version]
// Each Kernel Revision of GSD is named after a random object
// For example 'Chair' each release has a subversion to it
// Full Example: GSD Chair v. 1.0 / Gsd Fan 1.0 etc..
#define GSD_KERNEL_RELEASE_NAME "Chair"
// Version String of the Kernel Release 
#define GSD_KERNEL_RELEASE_VERSION "0.1"



#endif