
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "wav.h"


static char* SIZE_SUFIX[] = { 
    "bytes", 
    "KB",
    "MB", 
    "GB", 
    "TB", 
    "PB", 
    "EB", 
    "ZB", 
    "YB" 
    };

void print_size(unsigned long long bytes) {
    
    int i = -1;
    {
      unsigned long long b = bytes;

      do {

        ++i;

        b = b / 1024;

      } while (b > 0 && i < sizeof(SIZE_SUFIX));
    }
    
    long double adjusted = (long double)bytes / (1ULL << (i * 10));
    
    printf("%Lf %s", adjusted, SIZE_SUFIX[i]);
    
}

void print_hex(uint8_t* buf, size_t size) {

    for(int i = 0; i < size; ++i) {
        printf("%02X ", buf[i]);
    }
    printf("\n");

}



int main(int argc, char*argv[]) {
    
    
    char* filename = "./file.wav";
    
    if(argc > 1) {
        
        filename = argv[1];
    }

    
    const char* FILENAME = filename;
    
    printf("Reading %s\n", FILENAME);
    FILE* handle = fopen(FILENAME , "r");
    
    if(!handle){
        fprintf(stderr, "Could not read file %s\n", FILENAME);
        return 1;
    }
    
    uint8_t buf[1024 * 1024 * 2];
    
    fread(buf, 1, sizeof(buf),handle);

    
    print_info(buf, sizeof(buf));
    

    // verify_wav(buf, sizeof(buf));


    print_hex(buf, 44);
    
    cleanup:
    fclose(handle);

}