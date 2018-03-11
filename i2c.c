#include <linux/i2c-dev.h>

int main(int argc, char ** argv) {
    int file = open("/dev/i2c-1", O_RDWR);
    if(file < 0) {
        fprintf(stderr, "could not open device\n");
	exit(1);
    }

    int addr = 0x20; 
}
