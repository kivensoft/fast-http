#include <stdio.h>
#include <inttypes.h>
#include "platform.h"
#include "str.h"

#ifndef __x86_64
#error must compiler by 64 bit
#endif

#define CONCAT(a, b) CONCAT1(a, b)
#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a##b

#define T(x) T2(typeof(x))
#define T2(x) #x

void output(const char* name, string_t* src) {
    printf("%s [%p] {ref: %u, len: %u, cap: %u, data: %s}\n", name, src, string_ref(src),
            string_len(src), string_cap(src), string_cstr(src));
}

int main() {
    const char *hw = "hello world";
    const char *name = ", kiven";
    string_t* s2 = string_from_cstr(name);
    string_t* str = string_from_cstr(hw);
    
    typeof(hw) a = "abc";
    typeof(1) b = 1024;
    printf("%s, %d, sizeof(size_t) = %" PRId64 "\n", a, b, sizeof(size_t));

    output("str ", str);

    string_t* str1 = string_add_ref(str);
    output("str1", str1);

    string_t* str2 = string_clone(str);
    output("str2", str2);

    string_t* str3 = string_cat(string_add_ref(str), name);
    output("str3", str3);

    // string_t* str4 = string_cats(string_add_ref(str), NULL, name, NULL);
    string_t* str4 = string_cats(string_add_ref(str), PP_ARGS(NULL, name, NULL));
    output("str4", str4);

    string_t* str5 = string_join(string_add_ref(str), s2);
    output("str5", str5);

    string_t* str6 = string_joins(string_add_ref(str), PP_ARGS(NULL, s2, NULL));
    output("str6", str6);

    string_releases(PP_ARGS(str, str1, str2, str3, str4, str5, str6));

    printf("%" PRId64 ", end\n", sizeof(string_t));
}