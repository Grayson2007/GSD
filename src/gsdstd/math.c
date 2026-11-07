#include <gsdstd/gsdmath.h>

struct s_iee754_32 {
        i32 mantissa;
        i32 exp;
        bool sign;
};
struct s_iee754_64 {
        i64 mantissa;
        i32 exponent;
        bool sign;
};

#define f32_bias 127
#define f64_bias 1023


float ftrunc(float x) {
        if(x == 0.0f) { return x;}
        union {
                float f;
                int i;
        } data;
        data.f = x;

        int man = data.i & 0x7FFFFF;
        int exp = (data.i >> 23 & 0xFF)-f32_bias;
        if(exp >= 23) { return x;}
        
        data.i &= ~((1 << exp)-1);
        return data.f;
}
float fmod(float a,float b); // computes a % b 
float fln(float x); // computes ln(x)
float flog2(float x); // 
float flogn(float x,float y); // computes logy(x) 
float fpow(float x,float y);
