#ifndef gsd_stdcall_h
#define gsd_stdcall_h
#include "gsd-common.h"
typedef uint kcallid;
status kerncall(kcallid function,int argc,void** argarr);
status querycall(char* callname,kcallid* outid);



#endif