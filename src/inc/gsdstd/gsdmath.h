#ifndef gsd_math_h
#define gsd_math_h
#include "stdtype.h"
#define PI 3.141592653589793
#define E 2.718281828459045
#define PIf 3.1415926535
#define Ef 2.718281828

#define square(x) x*x
#define cube(x) square(x)*x


float ftrunc(float x);
float fmod(float a,float b); // computes a % b 
float fln(float x); // computes ln(x)
float flog2(float x); // 
float flogn(float x,float y); // computes logy(x) 
float fpow(float x,float y);





#endif