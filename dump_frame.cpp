#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

void print_bits(uint32_t d) {
    int i, j;
    for (j=7;j>=0;j--) {
        printf("%u", (d >> j) & 1);
    }
}

int main() {
    int dest_base = 0x0f800000;
    int dh = open("/dev/mem", O_RDWR); 
    int size = 2985984;
    uint64_t * data = (uint64_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dh, dest_base); 

    uint64_t * data2 = (uint64_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dh, dest_base + size); 
    

    for(int i = 0; i < 100; i++) {
        //write(1, data, size);
        memcpy(data2, data, size);
    }

    /*
    for(int i = 0; i < size / 8; i++) {
        auto d = data[i];

        auto dd = 0;
        dd |= (d >> (0 * 12 + 4)) << (0 * 8);
        dd |= (d >> (1 * 12 + 4)) << (1 * 8);
        dd |= (d >> (2 * 12 + 4)) << (2 * 8);
        dd |= (d >> (3 * 12 + 4)) << (3 * 8);

        buf[i] = dd;

    }
    write(1, buf, size);
    */
}
