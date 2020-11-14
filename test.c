#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

int main(int argc, char** argv) {
    uint32_t ps = 1024;
    uint32_t a1 = (2500 + ps - 1) & ~(ps - 1);
    uint32_t a2 = (1025 + ps - 1) & ~(ps - 1);
    printf("%u, %u\n", a1, a2);
}
