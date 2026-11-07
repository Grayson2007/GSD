
#include <gsd-memory.h>
#include <gsdstd/gsdstd.h>
// ctx is a generic pointer for the putch function
// for a simple output to a serial this would be NULL but for things like dynamic string 
// copying or a strncat system this could point to a struct that has a capacity and a buff ptr 

enum strfmt_states {
        strfmt_copy,
        strfmt_flags,
        strfmt_minwidth,
        strfmt_percision,
        strfmt_conversion,
        strfmt_nfmt,
        strfmt_escape,
        strfmt_escape_unicode // unicode processing 
};

enum fmt_attrib {
        fmt_attrib_ljustify = (1 << 0),
        fmt_attrib_sign = (1 << 1),
        fmt_attrib_zeropad = (1 << 2),
        fmt_attrib_altform = (1 << 3),
        fmt_attrib_upper = (1 << 4),
};
#define conv_buffsz(T) 8*sizeof(T)



static size fmt_parse_int(char* fmt,int* out) {
        size n = 0;
        *out = 0;
        while(*fmt > '0' && *fmt < '9') {
                *out *= 10;
                n++;
                *out += (*fmt-'0');
                fmt++;
        }
        return n;
}




void strfmt(char* fmt,void* ctx,void(*putch)(void* ctx,char* seq),va_list argv) {
        int state = strfmt_copy;

        int fmt_attrib; 
        int fmt_minwidth;
        int fmt_percission;
        char unicode_buffer[4];
        bool fmt_long = false;



        while(*fmt)
        {
                switch (state)
                {
                        case strfmt_copy:
                                if(*fmt == '%') {

                                        fmt++;
                                        if(*fmt == '%') {
                                                putch(ctx,'%');
                                                fmt++;
                                                state = strfmt_copy;
                                                continue;
                                        }
                                        state = strfmt_flags;
                                        continue;
                                }

                                if(*fmt == '\\') {
                                        state = strfmt_escape;
                                        fmt++;
                                        continue;
                                }
                                size chsz = chrsz(fmt);
                                putch(ctx,fmt);
                                fmt += chsz;
                                continue;
                        case strfmt_flags:
 
                                switch(*fmt)
                                {
                                        case '+':
                                                fmt_attrib |= fmt_attrib_sign;
                                                fmt++;
                                                continue;
                                        case '-':
                                                fmt_attrib |= fmt_attrib_altform;
                                                fmt++;
                                                continue;
                                        case '.':
                                                fmt++;
                                                state = strfmt_percision;
                                                continue;
                                        case '#':
                                                fmt_attrib |= fmt_attrib_altform;
                                                fmt++;
                                                continue;
                                        case '0':
                                                fmt_attrib |= fmt_attrib_zeropad;
                                                fmt++;
                                                continue;
                                }
                                state = strfmt_minwidth;
                                fmt++;
                                continue;

                        case strfmt_minwidth:
                                fmt_minwidth = 0;
                                fmt += fmt_parse_int(fmt,&fmt_minwidth);
                                if(*fmt == '.') { fmt++; state = strfmt_percision; continue;}
                                state = strfmt_conversion;
                                continue;
                        case strfmt_percision:
                                fmt_percission = 0;
                                fmt += fmt_parse_int(fmt,&fmt_percission);
                                state = strfmt_conversion;
                                continue;
                        case strfmt_conversion: 
                                if(*fmt == 'l') 
                                {
                                        fmt_long = true;
                                        fmt++;
                                }


                                switch(*fmt) {
                                        case 'd' | 'i':

                                        case 'u' : 

                                        case 'f' :

                                        case  'g':
                                }

                                fmt_long = false;
                                state = strfmt_copy;
                                fmt++;
                                continue;

                        case strfmt_escape:
                                switch(*fmt) {
                                        case 'n':
                                                putch(ctx,"\n");
                                                break;
                                        case 'r':
                                                putch(ctx,"\r");
                                                break;
                                        case 't':
                                                putch(ctx,"\t");
                                                break;
                                        case '\\':
                                                putch(ctx,"\\");
                                                break;
                                        case 'b':
                                                putch(ctx,"\b");
                                                break;
                                        case 'u': // unicode 
                                                state = strfmt_escape_unicode;
                                                fmt++;
                                                continue;
                                }
                                fmt++;
                                state = strfmt_copy;
                                break;
                        case strfmt_escape_unicode:
                                

                                break;



                }
        }

}



