#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <curl/curl.h>
#include <pocketsphinx.h>
#include <cjson/cJSON.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

typedef enum{
    os_windows = 1, os_macos = 2, os_linux = 3
}OS;

bool verbose = false;

OS os = 0;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
void escape_json_string(const char *input, char *output, size_t output_size);


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
        16000

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
    deviceConfig.sampleRate       = 16000;
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
    /*
    TRANSCRIBING WITH POCKETSPHINX
    TRANSCRIBING WITH POCKETSPHINX
    TRANSCRIBING WITH POCKETSPHINX
    TRANSCRIBING WITH POCKETSPHINX
    TRANSCRIBING WITH POCKETSPHINX
    TRANSCRIBING WITH POCKETSPHINX
    */


     ps_decoder_t *decoder;
     ps_config_t *config;
     FILE *fh;
     short *buf;
     size_t len, nsamples;
    //open the recording
     if ((fh = fopen("recording.wav", "rb")) == NULL){
        printf("Failed to open the recording");
        exit(1);
     }

     //get length of recording
     if (fseek(fh, 0, SEEK_END) < 0) printf("unable to find end of recording.wav"); 
     len = ftell(fh);
     rewind(fh);

     //initalize config from input file

     config = ps_config_init(NULL);
     ps_default_search_args(config);
     if (ps_config_soundfile(config, fh, "recording\\.wav") < 0){
        printf("unsupported input file");
        exit(1);
     }
    if ((decoder = ps_init(config)) == NULL){
        printf("Pocketsphinx decoder init failed\n");
        exit(1);
    }

    //allocate data

    len -= ftell(fh);

    if ((buf = malloc(len)) == NULL){
        printf("failed to allocate %d bytes", len);
        exit(1);
    }
    // read the input

    nsamples = fread(buf, sizeof(buf[0]), len / sizeof(buf[0]), fh);
    if (nsamples != len / sizeof(buf[0])){
        printf("Unable to read %d samples", len / sizeof(buf[0]));
        exit(1);
    }

    // transcribe the recording

    if (ps_start_utt(decoder) < 0){
        printf("failed to start transcribing");
        exit(1);
    }
    if(ps_process_raw(decoder, buf, nsamples, FALSE, TRUE) < 0){
        printf("ps_process_raw() failed\n");
        exit(1);
    }
    if (ps_end_utt(decoder) < 0){
        printf("Failed to end the transcription process");
        exit(1);
    }
// Get the transcription result
const char *hyp = ps_get_hyp(decoder, NULL);
    if (hyp == NULL) {
        printf("No transcription available\n");
        exit(1);
    }

// Copy to query buffer
char query[1028];
strncpy(query, hyp, sizeof(query) - 1);
query[sizeof(query) - 1] = '\0';

printf("%s\n", query);



// Use a proper multi-line string
char prompt[2461] = "You are a voice assistant with the following capabilities:\n"
"- Answer questions concisely\n"
"- Set timers, reminders, and alarms\n"
"- Provide information lookups\n"
"- Perform system integrations (except those requiring admin permissions, note that you can not actually perform these integrations, but something else will handle them, just pretend like they got handled)\n";


curl_global_init(CURL_GLOBAL_ALL);
CURL *handle = curl_easy_init();

curl_easy_setopt(handle, CURLOPT_URL, "https://ai.hackclub.com/proxy/v1/chat/completions");

struct curl_slist *headers = NULL;
char auth_header[256];
char *api_key = getenv("HC_AI_API_KEY");

//verbose ? printf("%s", api_key) : printf("nothing");

snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
headers = curl_slist_append(headers, auth_header); 
headers = curl_slist_append(headers, "Content-Type: application/json");

char escaped_prompt[5000];
char escaped_query[5000];
escape_json_string(prompt, escaped_prompt, sizeof(escaped_prompt));
escape_json_string(query, escaped_query, sizeof(escaped_query));

char json_data[4096];

snprintf(json_data, sizeof(json_data), 
"{\n"
"  \"model\": \"google/gemini-3-flash-preview\",\n"
"  \"messages\": [\n"
"    {\n"
"      \"role\": \"system\",\n"
"      \"content\": \"%s\"\n"
"    },\n"
"    {\n"
"      \"role\": \"user\",\n"
"      \"content\": \"%s\"\n"
"    }\n"
"  ]\n"
"}", 
    escaped_prompt, escaped_query);

curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
curl_easy_setopt(handle, CURLOPT_POSTFIELDS, json_data);
curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);

CURLcode res = curl_easy_perform(handle);

if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
}



// Clean up
free(buf);
fclose(fh);
ps_free(decoder);
ps_config_free(config);

return 0;
}




 void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
    ma_encoder* pEncoder = (ma_encoder*)pDevice->pUserData;

    if (pInput != NULL) {
        ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, NULL);
    }

    (void)pOutput;
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    printf("%.*s", (int)realsize, (char *)contents);
    return realsize;
}

void escape_json_string(const char *input, char *output, size_t output_size){

size_t j = 0;

for(size_t i = 0; input[i] != '\0' && j < output_size - 2; i++){
    switch (input[i]){
        case '"':
        case '\\':
            output[j++] = '\\';
            output[j++] = input[i];
            break;
        case '\n':
            output[j++] = '\\';
            output[j++] = 'n';
            break;
        case '\r':
            output[j++] = '\\';
            output[j++] = 'r';
            break;
        case '\t':
            output[j++] = '\\';
            output[j++] = 't';
            break;
        default:
            output[j++] = input[i];
            break;
        }
    }
    output[j] = '\0';
}







