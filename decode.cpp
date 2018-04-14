#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <vector>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define min(a,b) (a) < (b) ? (a) : (b);

struct uint12_t {
    unsigned data: 12;
};


void print_bits(uint12_t d)
{
    int i, j;
    for (j=11;j>=0;j--) {
        printf("%u", (d.data >> j) & 1);
    }
//    printf("\n");
}

uint12_t get_shifted(uint32_t * in, int offset, int lane = 0) {
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

std::vector<uint12_t> get_words(uint32_t * data, uint64_t this_word, uint64_t this_offset, uint64_t count, int lane = 0) {
    std::vector<uint12_t> words;

    auto next_offset = this_offset;
    auto next_word = this_word;

    for(int i = 0; i < count; i++) {
        next_offset += 12;
        next_word += (next_offset / 8);
        next_offset %= 8;
/*
        printf("next_word %d\n", next_word);
        printf("next_offset %d\n", next_offset);
 */       
        words.push_back(get_shifted(&data[next_word], next_offset, lane));
    }

    return words;
}

int main(int argc, char** argv) {
    auto filename = "dump_solid_color";

    if(argc == 2) {
        filename = argv[1];
    }

    auto file = fopen(filename, "r");
    if(!file) {
        fprintf(stderr, "could not open file\n");
        exit(-1);
    }
    fseek(file, 0, SEEK_END); 
    auto size = ftell(file);
    rewind(file);

    uint32_t * pre_data = (uint32_t *) malloc(size);
    fread(pre_data, 1, size, file);
    uint32_t * data = (uint32_t *) calloc(size, sizeof(uint32_t));

    auto found = 0;
    auto lane_length = size / 4;
    auto number_of_lanes = 4;

    auto d = 0;
    /*
    for(int i = 0; i < size / 8; i += 3) {
        uint32_t a = pre_data[i + 0];
        uint32_t b = pre_data[i + 1];
        uint32_t c = pre_data[i + 2];
        int j = 0;

        if(i % 30000 == 0) {
            printf("\r%d vs %d", i, size);
        }


        for(; j < 8; j++) {
            for(int l = 0; l < 4; l++) {
                data[l * lane_length + d].data |= ((a >> (number_of_lanes * j + l)) & 0x1) << j;
            }
        }

        for(; j < 12; j++) {
            for(int l = 0; l < 4; l++) {
                data[l * lane_length + d].data |= ((b >> (number_of_lanes * (j - 4) + l)) & 0x1) << j;
            }
        }

        d += 1;

        for(j = 0; j < 4; j++) {
            for(int l = 0; l < 4; l++) {
                data[l * lane_length + d].data |= ((b >> (number_of_lanes * j + l)) & 0x1) << j;
            }
        }

        for(; j < 12; j++) {
            for(int l = 0; l < 4; l++) {
                data[l * lane_length + d].data |= ((c >> (number_of_lanes * (j - 4) + l)) & 0x1) << j;
            }
        }

        d += 1;


//        data[d++] = ((a << 4)) & 0xff0 | (b & 0xf)
//        data[d++] = ((b << 8) & 0xf00) | (c & 0xff)
    }
    */

    printf("\n");

    bool printed = false;

    int lanes_found = 0;

    int lane_offsets[4][2] = { { 0 } };

    printf("lane_length %d\n", lane_length);
    for(int i = 0; i < size / 4; i++) {
        for(int offset = 0; offset < 8; offset++) {
            for(int lane = 0; lane < 4; lane++) {
                auto v = get_shifted(&pre_data[i], offset, lane);
//                if(v.data != 0) {
                    print_bits(v);
                    printf(" ");
//                }
                if(v.data == 0xfff) {
                    auto words = get_words(pre_data, i, offset, 3, lane);
                    if(words[0].data == 0 && words[1].data == 0) {
                        printed = true;
                        printf("| lane %d, found %d, offset %d: ", lane, i, offset);
                        print_bits(v);
                        for(auto word : words) {
                            printf(" ");
                            print_bits(word);
                        }
                        printf(" | ");
                        
                        lanes_found += 1;
                        lane_offsets[lane][0] = i;
                        lane_offsets[lane][1] = offset;
                        
                        if(lanes_found == 4) goto decode;
                    }
                   
//                    auto next_offset = 12 + offset;
//                    auto next_word = i + (next_offset / 8);
//                    next_offset %= 8;
//
//                    printf("next "); print_bits(get_shifted(&pre_data[next_word], next_offset));

                }
            }
//            if(printed) {
                printf("\n");
                printed = false;
//            }
        }
    }

decode:
    long long remaining_size = (1LL << (sizeof(int) * 8 - 1)) - 1;

    for(int i = 0; i < 4; i++) {
        remaining_size = min(remaining_size, (size - lane_offsets[i][0]) / 6); // 4 lane, 3 words = 2 data => 2/12 = 1/6
    }

    remaining_size -= 100; // be save factor

    auto words_lane0 = get_words(pre_data, lane_offsets[0][0], lane_offsets[0][1], remaining_size, 0);
    auto words_lane1 = get_words(pre_data, lane_offsets[1][0], lane_offsets[1][1], remaining_size, 1);
    auto words_lane2 = get_words(pre_data, lane_offsets[2][0], lane_offsets[2][1], remaining_size, 2);
    auto words_lane3 = get_words(pre_data, lane_offsets[3][0], lane_offsets[3][1], remaining_size, 3);
    
    int width = 0;
    int height = 0;

    char * image = (char *) malloc(sizeof(char) * size);
    int image_position = 0;

    for(int i = 4; i < remaining_size; i++) {
        if(words_lane0[i - 4].data == 0xfff && words_lane0[i - 3].data == 0 && words_lane0[i - 2].data == 0 && words_lane0[i - 1].data == 1) {
            width = 0;
            while(!(words_lane0[i].data == 0xfff && words_lane0[i + 1].data == 0 && words_lane0[i + 2].data == 0)) {
                image[image_position++] = words_lane0[i].data >> 4;
                image[image_position++] = words_lane1[i].data >> 4;
                image[image_position++] = words_lane2[i].data >> 4;
                image[image_position++] = words_lane3[i].data >> 4;
                i++;
                width += 4;
/*
        print_bits(words_lane0[i]);
        printf(" ");
        print_bits(words_lane1[i]);
        printf(" ");
        print_bits(words_lane2[i]);
        printf(" ");
        print_bits(words_lane3[i]);
        printf("\n");
        */
            }
            height++;
        }
/*
        print_bits(words_lane0[i]);
        printf(" ");
        print_bits(words_lane1[i]);
        printf(" ");
        print_bits(words_lane2[i]);
        printf(" ");
        print_bits(words_lane3[i]);
        printf("\n");
        */
    }

    stbi_write_png("test.png", width, height, 1, image, 0);
     

//    auto words = get_words(pre_data, 0, 0, size / 4);        
        /*
        if(data[i].data == 0xfff && data[i + 1].data == 0) {
            print_bits(2, &data[i]); 
            print_bits(2, &data[i + 1]); 
            print_bits(2, &data[i + 2]); 
        }
//        if(data[i] == 0) {
//            if(found != i - 1) found = i;
//            else printf("found %d, %d\n", data[i], i);
//        }
    }
    */
}

