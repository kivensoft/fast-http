#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "str.h"
#include "list.h"


int main(int argc, char** argv) {
    char buf[] = "12345";
    char *p = &buf[2];
    printf("%c,%c\n", *p, p[-1]);
}
