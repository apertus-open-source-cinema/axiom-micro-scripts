#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>

void print_bits(uint32_t d) {
    int i, j;
    for (j=7;j>=0;j--) {
        printf("%u", (d >> j) & 1);
    }
}

int main() {
    int dest_base = 0x0f800000;
    int dh = open("/dev/mem", O_RDWR | O_SYNC); 
    int size = 0x400000 * 4;
    uint32_t * data = (uint32_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dh, dest_base); 

    for(int i = 0; i < size; i++) {
        auto d = data[i];
        printf("0x%x    0x%x\n", dest_base + 4 * i, d);
    }
}
