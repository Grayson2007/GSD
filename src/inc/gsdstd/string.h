#ifndef string_h
#define string_h
#include "stdtype.h"
size gsd_strlen(char* str); // returns the size in bytes of this string
size gsd_lstrlen(char* str); // returns the logical string length 
// meaning the amount of logical characters in the string 
size gsd_strspn(char* str,char* spn);
size gsd_strcspn(char* str,char* spn);
char* gsd_strchr(char* str,char* seq);
char* gsd_strnchr(char* str,char* seq,size n);
int gsd_strcmp(char* a,char* b,size n);
size gsd_utf8chrsz(char* seq); // returns either 1,2,3,4 or 0 if invalid 


void strfmt(char* fmt,void* ctx,void(*putch)(void* ctx,char* seq),va_list argv);
void printfmt(char* fmt,...); // prints a strfmt string to whatever output
void strbfmt(char* fmt,char* buff,size buffcap,size* outsz,...);  // Runs strfmt and copies the result in a buffer 
char* strhfmt(char* fmt,...); // heapstrfmt (Dynamically creates a new string in the heap via strfmt)


#define chr_inbound(chr,min,max) chr >= min && chr <= max
#define chr_isdigit(chr) chr_inbound(chr,'0','9')
#define chr_isxdigit(chr) isdigit(chr) || chr_inbound(chr,'a','f')
#define chr_isXdigit(chr) isdigit(chr) || chr_inbound(chr,'A','F')
#define chr_isXxDigit isdigit(chr) || chr_inbound(chr,'a','f') || chr_inbound(chr,'A','F')
#define chr_isAlpha(chr) chr_inbound(chr,'a','z') || chr_inbound(chr,'A','Z')
#define chr_isWhitespace(chr) chr == ' ' || chr == '\n' || chr == '\t'
#define STR_EXP(tok) #tok
#define M_STRINGIFY(tok) STR_EXP(tok)



#endif