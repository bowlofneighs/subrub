#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>


#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

typedef enum{
    os_windows = 1, os_macos = 2, os_linux = 3
}OS;

bool verbose = false;

OS os = 0;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);





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
            printf("Windows OS detected\n");
        }
        else if(os == os_macos){
            printf("macOS detected\n");
        }
        else if(os == os_linux){
            printf("Linux OS detected\n");
        }
    }


/*
TIME FOR THE RECORDING PART
TIME FOR THE RECORDING PART
TIME FOR THE RECORDING PART
TIME FOR THE RECORDING PART
TIME FOR THE RECORDING PART
TIME FOR THE RECORDING PART
*/
    ma_device_config deviceConfig;
    ma_device device;
    ma_encoder encoder;
    ma_encoder_config encoderConfig;

    encoderConfig = ma_encoder_config_init(
        ma_encoding_format_wav,
        ma_format_s16,
        1,
        44100

    );
     if (ma_encoder_init_file("recording.wav", &encoderConfig, &encoder) != MA_SUCCESS) {
        printf("Failed to create WAV file.\n");
        return 1;
    }
    else if (verbose){
        printf("created WAV file\n");
    }


    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format   = ma_format_s16;
    deviceConfig.capture.channels = 1;
    deviceConfig.sampleRate       = 44100;
    deviceConfig.dataCallback     = data_callback;
    deviceConfig.pUserData        = &encoder;

     if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to initialize capture device.\n");
        ma_encoder_uninit(&encoder);
        return 1;
    }
    else if (verbose){
        printf("initialized capture device\n");
    }

        if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start device.\n");
        ma_device_uninit(&device);
        ma_encoder_uninit(&encoder);
        return 1;
    }
    else if (verbose){
        printf("Started device\n");
    }
        printf("press enter to stop recording\n");
        getchar();

    ma_device_uninit(&device);
    ma_encoder_uninit(&encoder);

    printf("Done.\n");




}

 void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
    ma_encoder* pEncoder = (ma_encoder*)pDevice->pUserData;

    if (pInput != NULL) {
        ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, NULL);
    }

    (void)pOutput; // Not used for recording
}


