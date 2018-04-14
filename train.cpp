#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <time.h>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

#define min(a,b) (a) < (b) ? (a) : (b);

struct uint12_t {
    unsigned data: 12;
};


uint12_t lane_values[4] = { { 0b011110001001 }, { 0b100100011111 }, { 0b110010101111 }, { 0b101101010011 } };

#define S2MM_CONTROL_REGISTER 0x30
#define S2MM_STATUS_REGISTER 0x34
#define S2MM_DESTINATION_ADDRESS 0x48
#define S2MM_LENGTH 0x58
#define DMA_MEMORY_BASE 0x0f800000
#define DMA_AXI_ADDRESS 0x40400000
#define TLAST_GEN_ADDRESS 0x41200000
#define DMA_MAX_BLOCKSIZE (1 << 22)
// 200 * 1024 * 1024
#define DMA_BUFFER_SIZE 209715200 
#define DELAY_CONTROL_ADDRESS 0x41210000


void print_bits(uint12_t d) {
    int i, j;
    for (j=11;j>=0;j--) {
        printf("%u", (d.data >> j) & 1);
    }
}

uint12_t get_shifted(volatile uint32_t * in, int offset, int lane = 0) {
    assert(offset < 8); 

    uint12_t out;
    out.data = 0;
    
    int bitpos = 0;
    
    for(int i = offset; i < 8; i++) {
        out.data |= ((in[0] >> (lane + 4 * i)) & 1) << bitpos++;
    }

    int i = 0;
    while(bitpos < 12 && i < 8) {
        out.data |= ((in[1] >> (lane + 4 * i++)) & 1) << bitpos++;
    } 
    
    i = 0;
    while(bitpos < 12) {
        out.data |= ((in[2] >> (lane + 4 * i++)) & 1) << bitpos++;
    } 

    return out;
}

std::vector<uint12_t> get_words(volatile uint32_t * data, uint64_t this_word, uint64_t this_offset, uint64_t count, int lane = 0) {
    std::vector<uint12_t> words;

    auto next_offset = this_offset;
    auto next_word = this_word;

    for(int i = 0; i < count; i++) {
        next_offset += 12;
        next_word += (next_offset / 8);
        next_offset %= 8;

        words.push_back(get_shifted(&data[next_word], next_offset, lane));
    }

    return words;
}


uint32_t * physical_address_pointer(off_t address, off_t length = 65536) {
    int dev_mem = open("/dev/mem", O_RDWR | O_SYNC); // open /dev/mem which represents the whole physical memory
    return (uint32_t *) mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, dev_mem, address); // memory map physical address
}

void sleep_ns(long ns) {
    timespec ts = { 0, ns };
    nanosleep(&ts, NULL);
}

void sleep_ms(long ms) {
    sleep_ns(ms * 1000 * 1000);
}

// fuck c standard i need memset with volatile
void memset(volatile void * data, char pattern, size_t size) {
    volatile char * data_bytewise = (volatile char *) data;
    for(size_t i = 0; i < size; i++) {
        data_bytewise[i] = pattern;
    }
}

struct dma_control {
    volatile uint32_t * dma_registers;
    volatile uint32_t * tlast_gen;
    volatile uint32_t * dma_buffer;
    int devmem;

    dma_control() {
        // memory map AXI lite register block
        dma_registers = physical_address_pointer(DMA_AXI_ADDRESS); 
        // memory map tlast generator
        tlast_gen = physical_address_pointer(TLAST_GEN_ADDRESS);
        // memory map destination address
        dma_buffer = physical_address_pointer(DMA_MEMORY_BASE, DMA_BUFFER_SIZE);
    }

    volatile uint32_t * transfer(size_t length) {
        size_t size = length;
        size_t blksize = length > DMA_MAX_BLOCKSIZE ? DMA_MAX_BLOCKSIZE : length;

        // reset dma
        set(S2MM_CONTROL_REGISTER, 4);

        // clear destination block
        memset(dma_buffer, 1, size); 

        // setup tlast generator (counts in 32bit words => bytes to 32bit words)
        // the tlast generator hangs at the second port of the axi gpio 
        // -> use third word to set data (second word is random)
        tlast_gen[2] = blksize / 4;
        // give the axi lite slave some time, 
        // timing requirements fail during implementation 
        // (whatever that actually means for this usecase)
        sleep_ms(1);


        // single transfer to get clean pipeline state and shit
        transfer(DMA_MEMORY_BASE, blksize);

        
        // too lazy to do the math on last bit, just save the stuff from the loop
        // (just a bunch ouf modulo, but off by one errors suck
	    int dest = DMA_MEMORY_BASE - blksize;

        // transfer all whole blocks first
        for(int pos = 0; pos < size / blksize; pos++) {
	        dest = DMA_MEMORY_BASE + pos * blksize;
	        transfer(dest, blksize);
        } 
        
        // transfer last bit which is smaller than the blocksize
        if(size % blksize != 0) {
            // setup tlast generator (counts in 32bit words => bytes to 32bit words)
            // the tlast generator hangs at the second port of the axi gpio 
            // -> use third word to set data (second word is random)
            tlast_gen[2] = (size % blksize) / 4;
            // give the axi lite slave some time, 
            // timing requirements fail during implementation 
            // (whatever that actually means for this usecase)
            sleep_ms(1);

            transfer(DMA_MEMORY_BASE - (size % blksize), size % blksize);
        }

        // return (virtual!) address of destination
        return dma_buffer;
    }

private:
    void set(int offset, unsigned int value) {
        dma_registers[offset>>2] = value;
    }
    
    uint32_t get(int offset) {
        return dma_registers[offset>>2];
    }
    
    void s2mm_status() {
        auto status = get(S2MM_STATUS_REGISTER);
        printf("Stream to memory-mapped status (0x%08x@0x%02x):", status, S2MM_STATUS_REGISTER);
        if (status & 0x00000001) printf(" halted"); else printf(" running");
        if (status & 0x00000002) printf(" idle");
        if (status & 0x00000008) printf(" SGIncld");
        if (status & 0x00000010) printf(" DMAIntErr");
        if (status & 0x00000020) printf(" DMASlvErr");
        if (status & 0x00000040) printf(" DMADecErr");
        if (status & 0x00000100) printf(" SGIntErr");
        if (status & 0x00000200) printf(" SGSlvErr");
        if (status & 0x00000400) printf(" SGDecErr");
        if (status & 0x00001000) printf(" IOC_Irq");
        if (status & 0x00002000) printf(" Dly_Irq");
        if (status & 0x00004000) printf(" Err_Irq");
        printf(" length: %d", get(S2MM_LENGTH));
        printf("\n");
    }
   
    /*
    void memdump(void* virtual_address, int byte_count) {
        char *p = virtual_address;
        int offset;
        for (offset = 0; offset < byte_count; offset++) {
            printf("%02x", p[offset]);
            if (offset % 4 == 3) { printf(" "); }
        }
        printf("\n");
    }
    */
    
    int s2mm_sync() {
        // for now this is just "dump" polling of the status register
        // this could be done more fancy with interrupts, but 
        // that's not necessary for simple things like this

        auto s2mm_status = get(S2MM_STATUS_REGISTER);
        while(!(s2mm_status & 0x2) && !(s2mm_status & 0x1)) {
            s2mm_status = get(S2MM_STATUS_REGISTER);
        }
    }
    
    void transfer(size_t dest, size_t size) {
        // halt DMA
        set(S2MM_CONTROL_REGISTER, 0);
       
        // write destination address
        set(S2MM_DESTINATION_ADDRESS, dest); 
       
        // start S2MM channel with all interrupts masked
        set(S2MM_CONTROL_REGISTER, 0x0001);
       
        // write S2MM transfer length
        set(S2MM_LENGTH, size);
       
        // wait for S2MM sychronization
        //
        // if this locks up make sure all memory ranges 
        // are assigned under address editor!
        s2mm_sync(); 
    }
};

struct hispi_decoder {
    uint32_t * get_image() { return NULL; }

    static std::vector<std::vector<uint12_t>> get_aligned_lanes(volatile uint32_t * raw_dump, size_t size) {
        auto found = 0;
        auto number_of_lanes = 4;

        auto d = 0;

        int lanes_found = 0; // bit x -> lane x found

        int lane_offsets[4][2] = { { 0 } };

        // go over the whole data in 32bit words
        for(int i = 0; i < size / 4; i++) {
            // go over each possible offset (8bits of data per lane per 32bit word)
            for(int offset = 0; offset < 8; offset++) {
                // go over each lane
                for(int lane = 0; lane < 4; lane++) {
                    auto v = get_shifted(&raw_dump[i], offset, lane);
                     
                    // check for possible sync code
                    if(v.data == 0xfff) {
                        auto words = get_words(raw_dump, i, offset, 3, lane);

                        // according to spec a zero run of >= 24bits 
                        // can not occour out of sync codes
                        if(words[0].data == 0 && words[1].data == 0) {
                            lanes_found |= 1 << lane;
                            lane_offsets[lane][0] = i;
                            lane_offsets[lane][1] = offset;
                            
                            if(lanes_found == 0xf) goto decode;
                        }
                    }
                }
            }
        }

    decode:
        long long remaining_size = (1LL << (sizeof(int) * 8 - 1)) - 1;

        for(int i = 0; i < 4; i++) {
            remaining_size = min(remaining_size, (size - lane_offsets[i][0]) / 6); // 4 lane, 3 words = 2 data => 2/12 = 1/6
        }

        remaining_size -= 1; // be save factor (there could be one word hanging)

        auto words_lane0 = get_words(raw_dump, lane_offsets[0][0], lane_offsets[0][1], remaining_size, 0);
        auto words_lane1 = get_words(raw_dump, lane_offsets[1][0], lane_offsets[1][1], remaining_size, 1);
        auto words_lane2 = get_words(raw_dump, lane_offsets[2][0], lane_offsets[2][1], remaining_size, 2);
        auto words_lane3 = get_words(raw_dump, lane_offsets[3][0], lane_offsets[3][1], remaining_size, 3);
        
        return { words_lane0, words_lane1, words_lane2, words_lane3 };
     }
};

struct delay_control {
    volatile uint32_t * control_register;

    delay_control() {
        control_register = physical_address_pointer(DELAY_CONTROL_ADDRESS);
    }

    void set(int lane, int taps) {
        auto mask = 0b11111 << lane * 5;
        taps &= 0b11111;
        taps <<= lane * 5;
        //                                                         |-> mask of reset bit 
        taps = ((control_register[0] & ~mask) | (taps & mask)) & 0b011111111111111111111;

        control_register[0] = (1 << 20) | taps;
        sleep_ms(2);
        control_register[0] = taps;

        auto actually_set = control_register[2];
        if(actually_set != taps) {
            fprintf(stderr, "failed to set delay taps, expected %d, got %d \n", taps, actually_set);
//            exit(-1);
        }
    }
};

bool data_good(int lane_number, volatile uint32_t * data, size_t size) {
    auto lane_data = hispi_decoder::get_aligned_lanes(data, size);
    auto lane = lane_data[lane_number];

    for(int i = 0; i < lane.size() - 4; i++) {
        if(lane[i + 0].data == 0xfff && lane[i + 1].data == 0x000
        && lane[i + 2].data == 0x000 && lane[i + 3].data == 0x001) {
            if(lane[i + 4].data != 0 && lane[i + 4].data != 0xfff) {
                printf("good value %d = ", lane[i + 4].data);
                print_bits(lane[i + 4]);
                printf("\n");
                return 1;
            } else {
                printf("near fit %d\n", lane[i + 4].data);
            }
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    constexpr uint32_t data_size = 512 * 1024; // 1 MB
    dma_control dma;
    delay_control delay;

    bool good_values[4][32] = { { 0 } };

    delay.set(0, 16);
    delay.set(1, 10);
    delay.set(2, 2);
    delay.set(3, 16);
    
/*    
    for(int lane = 3; lane < 4; lane++) {
        for(int tap = 0; tap < 32; tap++) {
            delay.set(lane, tap); 
            auto raw_data = dma.transfer(data_size); 

            printf("lane %d, tap %d\n", lane, tap);
            good_values[lane][tap] = data_good(lane, raw_data, data_size);
        }
    }
*/
    for(int lane = 0; lane < 4; lane++) {
        for(int tap = 0; tap < 32; tap++) {
            printf("%d", good_values[lane][tap]);
        }
        printf("\n");
    }
}
