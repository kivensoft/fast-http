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
    pstring_t s2 = string_from_cstr(name);
    pstring_t str = string_from_cstr(hw);
    
    typeof(hw) a = "abc";
    typeof(1) b = 1024;
    printf("%s, %d, sizeof(size_t) = %" PRId64 "\n", a, b, sizeof(size_t));

    output("str ", str);

    pstring_t str1 = string_copy(str);
    output("str1", str1);

    pstring_t str2 = string_clone(str);
    output("str2", str2);

    pstring_t str3 = string_cat(string_copy(str), name);
    output("str3", str3);

    // pstring_t str4 = string_cats(string_copy(str), NULL, name, NULL);
    pstring_t str4 = string_cats(string_copy(str), PP_ARGS(NULL, name, NULL));
    output("str4", str4);

    pstring_t str5 = string_join(string_copy(str), s2);
    output("str5", str5);

    pstring_t str6 = string_joins(string_copy(str), PP_ARGS(NULL, s2, NULL));
    output("str6", str6);

    string_frees(PP_ARGS(str, str1, str2, str3, str4, str5, str6));

    printf("%" PRId64 ", end\n", sizeof(string_t));
}