#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <portaudio.h>
#include <stdint.h>


#define SAMPLE_RATE       44100
#define NUM_CHANNELS      1
#define FRAMES_PER_BUFFER 512
#define RECORD_SECONDS    5
#define SAMPLE_FORMAT paInt16

typedef int16_t SAMPLE;

typedef struct{
    
}


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
    if(verbose){printf("SubRub is running in verbose mode");}

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

    Pa_Initialize();






 }


