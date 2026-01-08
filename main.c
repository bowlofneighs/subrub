#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>








bool verbose = false;

typedef enum{
    os_windows = 1, os_macos = 2, os_linux = 3
}OS;

OS os = 0;

 int main(int argc, char *argv[]){


    //detecting command line args
    for(int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0){
              verbose = true;
        }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
            printf("help text\n");
            return 0;
        }
        
    }


    //inform the user that verbose mode is enabled
    if(verbose){printf("SubRub is running in verbose mode\n");}

    //detect user OS

    #if defined(_WIN32) || defined(_WIN64)
        os = os_windows;
    #elif defined(__APPLE__) && defined(__MACH__)
        os = os_macos;
    #elif defined(__linux__)
        os = os_linux;
    #else 
        printf("failed to detect operating system"\n);
        return 1;
    #endif

// print OS for verbose mode
    if(verbose){
        if(os == os_windows){
            printf("Windows OS detected");
        }
        else if(os == os_macos){
            printf("macOS detected");
        }
        else if(os == os_linux){
            printf("Linux OS detected");
        }
    }

    ma_encoder encoder;
    ma_result result;

    result = ma_encoder_init_file("recorded.wav",
                                  ma_format_f32,
                                  1,
                                  44100,
                                  NULL,
                                  &encoder);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize encoder\n");
        return 1;
    }
    else if(verbose){
        printf("initialized encoder");
    }

 }


