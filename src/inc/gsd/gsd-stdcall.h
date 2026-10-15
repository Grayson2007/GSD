#ifndef gsd_stdcall_h
#define gsd_stdcall_h
#include "gsd-common.h"
typedef uint kcallid;
typedef uint kc_ctx;
typedef status(*kc_function)(int argc,void** argarr);
#define kc_ctx_gsd 0 
status kerncall(kcallid function,int argc,void** argarr);
status gsdctx_querycall(char* callname,kcallid* outid);
status gsdctx_inscall(char* callname,char* binpath,kcallid* outid);

#endif