#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <curl/curl.h>


#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

typedef enum{
    os_windows = 1, os_macos = 2, os_linux = 3
}OS;

bool verbose = false;

OS os = 0;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
 void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
int upload_to_whisper(const char *filename);

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
        char buffer[10];
            fgets(buffer, sizeof(buffer), stdin);

    ma_device_uninit(&device);
    ma_encoder_uninit(&encoder);

        const char *api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        printf("OPENAI_API_KEY not set. Please set it now: \n");
        exit(1);
    }

    printf("Transcribing with Whisper-1\n");
    upload_to_whisper("recording.wav");

    printf("Done.\n");
}

 void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
    ma_encoder* pEncoder = (ma_encoder*)pDevice->pUserData;

    if (pInput != NULL) {
        ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, NULL);
    }

    (void)pOutput; // Not used for recording
}

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata){
    fwrite(ptr, size, nmemb, stdout);
    return size * nmemb;
}

int upload_to_whisper(const char *filename)
{
    CURL *curl;
    CURLcode res;

    const char *api_key = getenv("OPENAI_API_KEY");

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to init curl\n");
        return 1;
    }

    /* ---------- headers ---------- */
    struct curl_slist *headers = NULL;
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header),
             "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    /* ---------- multipart form ---------- */
    curl_mime *form = curl_mime_init(curl);

    curl_mimepart *part;

    /* file */
    part = curl_mime_addpart(form);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, filename);

    /* model */
    part = curl_mime_addpart(form);
    curl_mime_name(part, "model");
    curl_mime_data(part, "whisper-1", CURL_ZERO_TERMINATED);

    /* optional: language */
    /*
    part = curl_mime_addpart(form);
    curl_mime_name(part, "language");
    curl_mime_data(part, "en", CURL_ZERO_TERMINATED);
    */

    curl_easy_setopt(curl, CURLOPT_URL,
        "https://api.openai.com/v1/audio/transcriptions");

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    /* ---------- perform ---------- */
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
    }

    /* ---------- cleanup ---------- */
    curl_mime_free(form);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? 0 : 1;
}





