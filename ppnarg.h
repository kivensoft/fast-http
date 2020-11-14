#pragma once
#ifndef __PPNARG_H__
#define __PPNARG_H__

#ifndef PP_NARG
#   define PP_ARGS(...) PP_NARG(__VA_ARGS__), __VA_ARGS__
#   define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#   define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#   define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,N,...) N
#   define PP_RSEQ_N() 32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
#endif

#ifndef CONNECT
#   define CONNECT(a, b) CONNECT1(a, b)
#   define CONNECT1(a, b) CONNECT2(a, b)
#   define CONNECT2(a, b) a##b
#   define EXPAND_NARG(name, ...) CONNECT(name, PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#endif

/*
#define STRING_FREE_1(x) string_free(x);
#define STRING_FREE_2(x1, x2) string_free(x1); string_free(x2);
#define STRING_FREE_3(x1, x2, x3) string_free(x1); string_free(x2); string_free(x3);

#define STRING_FREE(...) EXPAND_NARG(STRING_FREE_, __VA_ARGS__)

#include "str.h"

int main() {
    pstring_t a,b,c;
    STRING_FREE(a,b,c);
}
*/

#endif // __PPNARG_H__