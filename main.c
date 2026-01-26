#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <curl/curl.h>
#include <pocketsphinx.h>
#include <cjson/cJSON.h>
#include <espeak-ng/speak_lib.h>
#include <sys/stat.h>
#include <ctype.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #define MKDIR(path) mkdir(path, 0755)
#endif

typedef enum{
    os_windows = 1, os_macos = 2, os_linux = 3
}OS;

struct MemoryStruct {
    char *memory;
    size_t size;
};

typedef enum{
    TOKEN_IDENTIFIER,
    TOKEN_LBRACE, //{
    TOKEN_RBRACE, //}
    TOKEN_EQUALS, //=
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct{
    TokenType type;
    char *value; //what char it is
    int line;
} Token;

typedef struct{
    FILE *fp;
    int current_line;
    int current_char;
} Lexer;

int is_identifier_start(int c){
    //checks if c can start an identifier
    return isalpha(c) || c == '_';
}

int is_identifier_char(int c){
    //checks if c can be part of an identifier
    return isalnum(c) || c == '-' || c == '_' || c == '.';
}

int is_whitespace(int c){
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

Token make_token(TokenType type, char *value, int line){
    Token t;
    t.type = type;
    t.value = value;
    t.line = line;
    return t;
}

const char* token_type_to_string(TokenType type){
    switch (type){
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_LBRACE:     return "LBRACE";
        case TOKEN_RBRACE:     return "RBRACE";
        case TOKEN_EQUALS:     return "EQUALS";
        case TOKEN_EOF:        return "EOF";
        case TOKEN_ERROR:      return "ERORR";
        default:               return "UNKOWN";
    }
}

// LEXER FUNCS

Lexer* lexer_init(FILE *fp){
    Lexer *lex = malloc(sizeof(Lexer));
    lex->fp = fp;
    lex->current_line = 1;
    lex->current_char = ' ';
    return lex;
}

void lexer_free(Lexer *lex){
    free(lex);
}

int lexer_peek(Lexer *lex) {
    int c = fgetc(lex->fp);
    if (c != EOF) {
        ungetc(c, lex->fp);  // Put it back
    }
    return c;
}

int lexer_advance(Lexer *lex){
    int c = fgetc(lex->fp);

    if (c == '\n'){
        lex->current_line++;
    }
    return c;
}
void lexer_skip_whitespace(Lexer *lex){
    while(is_whitespace(lex->current_char)){
        lexer_advance(lex);
    }
}
void lexer_skip_comment(Lexer *lex){

    while(lex->current_char != '\n' && lex->current_char != EOF){
        lexer_advance(lex);
    }

    if (lex->current_char == '\n'){
        lexer_advance(lex);
    }
}

Token lexer_read_identifier(Lexer *lex){
    char buffer[256];
    int buf_pos = 0;
    int start_line = lex->current_line;


    buffer[buf_pos++] = lex->current_char;
    lexer_advance(lex);;

    while (is_identifier_char(lex->current_char)) {
        if (buf_pos >= 255){
            printf("Error in config file: Identifier too long at line %d\n", start_line);
            return make_token(TOKEN_ERROR, NULL, start_line);
        }
        buffer[buf_pos++] = lex->current_char;
        lexer_advance(lex);
    }

    buffer[buf_pos] = '\0';

    char *value = strdup(buffer);

    return make_token(TOKEN_IDENTIFIER, value, start_line);
}

Token lexer_next_token(Lexer *lex){
    for(;;){
        lexer_skip_whitespace(lex);

        if (lex->current_char == '#'){
            lexer_skip_comment(lex);
            continue;
        }

        break;
    }

    int line = lex->current_line;

    if (lex->current_char == EOF){
        return make_token(TOKEN_EOF, NULL, line);
    }


    switch (lex->current_char){
        case '{':
            lexer_advance(lex);
            return make_token(TOKEN_LBRACE, NULL, line);
        case '}':
            lexer_advance(lex);
            return make_token(TOKEN_RBRACE, NULL, line);
        case '=':
            lexer_advance(lex);
            return make_token(TOKEN_EQUALS, NULL, line);
    }

    if (is_identifier_start(lex->current_char)){
        return lexer_read_identifier(lex);
    }

    printf("Error: Unexpected character '%c' (ASCII %d) at line %d", lex->current_char, lex->current_char, line);
    return make_token(TOKEN_ERROR, NULL, line);
}

typedef struct{
    char *api_key;
    char *url;
    char *model;
} AIConfig;


typedef struct{
    char *name;
    char *keyword;
    char* command;
} KeywordEntry;

typedef struct{
    AIConfig ai;
    KeywordEntry *keywords;
    int keyword_count;
    int keyword_capacity;
} Config;

typedef struct{
    Lexer *lexer;
    Token current_token;
    Token peek_token;
    Config *config;
} Parser;

void parser_advance(Parser *p){
    if(p->current_token.value){
        free(p->current_token.value);
    }

    p->current_token = lexer_next_token(p->lexer);
}

void parser_expect(Parser *p, TokenType expected) {
    if (p->current_token.type != expected) {
        fprintf(stderr, "Error in the config file on line %d: Expected %s but got %s\n",
                p->current_token.line,
                token_type_to_string(expected),
                token_type_to_string(p->current_token.type));
        exit(1);
    }
    parser_advance(p);  // Consume the token
}

int parser_check(Parser *p, TokenType type){ //checks current token type
    return p->current_token.type == type;
}


void parse_ai_section(Parser *p){
    while (!parser_check(p, TOKEN_RBRACE)){
        if (!parser_check(p, TOKEN_IDENTIFIER)){
            printf("Error on line %d of the config file: expected property name\n", 
                    p->current_token.line);
            exit(1);
        }
        char *key = strdup(p->current_token.value);
        parser_advance(p);
        
        parser_expect(p, TOKEN_EQUALS);  // ✓ Add this
        
        if (!parser_check(p, TOKEN_IDENTIFIER)){  // ✓ Add this
            printf("Error on line %d: expected value\n", p->current_token.line);
            exit(1);
        }
        
        char *value = strdup(p->current_token.value);  // ✓ Add this
        parser_advance(p);  // ✓ Add this
        
        if (strcmp(key, "api_key") == 0){
            p->config->ai.api_key = value;
        } else if (strcmp(key, "url") == 0){
            p->config->ai.url = value;
        } else if (strcmp(key, "model") == 0){
            p->config->ai.model = value;
        } else {
            printf("Warning: unknown ai property \"%s\" in the config file at line %d\n", key, p->current_token.line);
            free(value);
        }
        
        free(key);  // ✓ Don't forget this
    }
}



void add_keyword_entry(Config *cfg, KeywordEntry entry) {
    // Grow array if needed
    if (cfg->keyword_count >= cfg->keyword_capacity) {
        cfg->keyword_capacity = cfg->keyword_capacity == 0 ? 4 : cfg->keyword_capacity * 2;
        cfg->keywords = realloc(cfg->keywords, 
                               cfg->keyword_capacity * sizeof(KeywordEntry));
        if (cfg->keywords == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for keywords\n");
            exit(1);
        }
    }
    
    cfg->keywords[cfg->keyword_count++] = entry;
}

void parse_keyword_block(Parser *p) {
    // Read the keyword name (e.g., "brightness-down")
    if (!parser_check(p, TOKEN_IDENTIFIER)) {
        fprintf(stderr, "Error line %d: Expected keyword name\n",
                p->current_token.line);
        exit(1);
    }
    
    char *name = strdup(p->current_token.value);
    parser_advance(p);
    
    parser_expect(p, TOKEN_LBRACE);
    
    // Create a new keyword entry
    KeywordEntry entry = {0};
    entry.name = name;
    
    // Read properties inside this keyword block
    while (!parser_check(p, TOKEN_RBRACE)) {
        char *key = strdup(p->current_token.value);
        parser_advance(p);
        
        parser_expect(p, TOKEN_EQUALS);
        
        char *value = strdup(p->current_token.value);
        parser_advance(p);

        if (strcmp(key, "keyword") == 0) {
            entry.keyword = value;
        } else if (strcmp(key, "command") == 0) {
            entry.command = value;
        } else {
            fprintf(stderr, "Warning: Unknown keyword property '%s'\n", key);
            free(value);
        }
        
        free(key);
    }
    
    parser_expect(p, TOKEN_RBRACE);
    
    // Add entry to the config's keyword array
    add_keyword_entry(p->config, entry);
}

void parse_keywords_section(Parser *p) {
    // Keep reading keyword blocks until '}'
    while (!parser_check(p, TOKEN_RBRACE)) {
        parse_keyword_block(p);
    }
}

void parse_section(Parser *p) {
    // We expect an identifier for the section name
    if (!parser_check(p, TOKEN_IDENTIFIER)) {
        fprintf(stderr, "Error line %d: Expected section name\n", 
                p->current_token.line);
        exit(1);
    }
    
    char *section_name = strdup(p->current_token.value);
    parser_advance(p);
    
    // Expect opening brace
    parser_expect(p, TOKEN_LBRACE);
    
    // Parse content based on section name
    if (strcmp(section_name, "AI") == 0) {
        parse_ai_section(p);
    } else if (strcmp(section_name, "Keywords") == 0) {
        parse_keywords_section(p);
    } else {
        fprintf(stderr, "Error line %d: Unknown section '%s'\n",
                p->current_token.line, section_name);
        exit(1);
    }
    
    // Expect closing brace
    parser_expect(p, TOKEN_RBRACE);
    
    free(section_name);
}

void parse_config(Parser *p){ //parse the file
    parser_advance(p);

    while (!parser_check(p, TOKEN_EOF)){
        parse_section(p);
    }
}

Config* parse_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    
    Lexer *lexer = lexer_init(fp);
    lexer_advance(lexer);  // Prime the lexer
    
    Config *config = calloc(1, sizeof(Config));
    
    Parser parser = {
        .lexer = lexer,
        .config = config
    };
    
    parse_config(&parser);
    
    lexer_free(lexer);
    fclose(fp);
    
    return config;
}




bool verbose = false;
bool dry;

OS os = 0;
void speak(const char *text);
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
void escape_json_string(const char *input, char *output, size_t output_size);

int main(int argc, char *argv[]){

    //detect user OS

    #if defined(_WIN32) || defined(_WIN64)
        os = os_windows;
    #elif defined(__APPLE__) && defined(__MACH__)
        os = os_macos;
    #elif defined(__linux__)
        os = os_linux;
    #else 
        printf("failed to detect operating system\n");
        return 1;
    #endif


    //detecting command line args
    for(int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0){
              verbose = true;
        }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
            printf("help text\n");
            return 0;
        }
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dry-run") == 0){
            dry = true;
            printf("subrub is running in dry mode\n");
        }
        if (strcmp(argv[i], "-C") == 0 || strcmp(argv[i], "--config") == 0){
            char confirmation = '\0';
            char config_dir[1024];
            char config_path[1024];
            char big_config_dir[1024];
            printf("Generate an example config file?\n"
                   "WARNING: WILL OVERWRITE ANY EXISTING CONFIG FILE  [y/N] ");
            scanf("%c", &confirmation);
            if (confirmation == 'y'){
                if(os == os_linux || os == os_macos){
                    const char *home = getenv("HOME");

                    if (home == NULL){
                        printf("failed to get the $HOME environment variable\n");
                        exit(1);
                    }

                    snprintf(config_dir, sizeof(config_dir), "%s/.config/subrub", home);
                    snprintf(big_config_dir, sizeof(big_config_dir), "%s/.config", home);
                    snprintf(config_path, sizeof(config_path), "%s/.config/subrub/subrub.conf", home);
                    
                    MKDIR(big_config_dir);
                    MKDIR(config_dir); //fails silently if dir exists

                    printf("generating an example config file at ~/.config/subrub/subrub.conf\n");
                    FILE *config;
                    config = fopen(config_path, "w");
                    if (config == NULL){
                        printf("failed to create ~/.config/subrub/subrub.conf\n");
                        exit(1);
                    }
                    fprintf(config, "#api key\nAPI_KEY = example LOL\n#AI\nAI = google/gemini-3-flash-preview\n");
                    fclose(config);
                }
                else if(os == os_windows){
                    const char *appdata = getenv("APPDATA"); //not sure abt this need to test on windows
                    if (appdata == NULL){
                        printf("failed to get appdata environment variable\n");
                        exit(1);
                    }
                    snprintf(config_dir, sizeof(config_dir), "%s\\subrub", appdata);
                    MKDIR(config_dir);
                    snprintf(config_path, sizeof(config_path), "%s\\subrub\\subrub.conf", appdata);
                    FILE *config;
                    config = fopen(config_path, "w");
                    if(config == NULL){
                        printf("failed to open %%APPDATA%%\\subrub\\subrub.conf\n");
                        exit(1);
                    }
                    printf("generating an example config file at %APPDATA%\\subrub\\subrub.conf\n");
                    fprintf(config, "#api key\nAPI_KEY = example-LOL\n#AI\nAI = google/gemini-3-flash-preview\n");
                    fclose(config);
                }
            }
            exit(0);
        }     
    }


    //inform the user that verbose mode is enabled
    if(verbose){printf("SubRub is running in verbose mode\n");}



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

    char config_path[128];

    if (os == os_windows){
        char *appdata = getenv("APPDATA");
        if(appdata == NULL){
            printf("failed to get environment variable APPDATA");
            exit(1);
        }

        snprintf(config_path, sizeof(config_path), "%s\\subrub\\subrub.conf", appdata);
    }
    if (os == os_linux || os == os_macos){
        char *home = getenv("HOME");
        if (home == NULL){
            printf("failed to get HOME environment variable");
            exit(1);
        }
        snprintf(config_path, sizeof(config_path), "%s/.config/subrub/subrub.conf", home);
    }

    parse_file(config_path);





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

    if (!dry){

        // Use a proper multi-line string
        char prompt[2461] = "You are a voice assistant with the following capabilities:\n"
        "- Answer questions concisely\n"
        "- Set timers, reminders, and alarms\n"
        "- Provide information lookups\n"
        "- Perform system integrations (except those requiring admin permissions, note that you can not actually perform these integrations, but something else will handle them, just pretend like they got handled)\n"
        "- the transcription library is not so accurate, so try your best to handle nonsensical sentences by looking at other words that sound similar\n";


        curl_global_init(CURL_GLOBAL_ALL);
        CURL *handle = curl_easy_init();
        struct MemoryStruct chunk;
            chunk.memory = malloc(1);  // Will be grown as needed by realloc
        chunk.size = 0;

        curl_easy_setopt(handle, CURLOPT_URL, "https://ai.hackclub.com/proxy/v1/chat/completions");
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&chunk);

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
    //    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);

        CURLcode res = curl_easy_perform(handle);

        if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        if(verbose){
            printf("%s\n\n", chunk);
        }
        cJSON *root = cJSON_Parse(chunk.memory);
        if(root == NULL){
            const char *error_ptr = cJSON_GetErrorPtr();
        }

        cJSON *choices = cJSON_GetObjectItem(root, "choices");

        if(!cJSON_IsArray(choices)) {
            cJSON_Delete(root);
        }

        cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
        if(first_choice == NULL){
            cJSON_Delete(root);
        }

        cJSON *message = cJSON_GetObjectItem(first_choice, "message");
        if(!cJSON_IsObject(message)){
            cJSON_Delete(root);
        }

        cJSON *content = cJSON_GetObjectItem(message, "content");
        if (cJSON_IsString(content) && (content->valuestring != NULL)){
            printf("%s\n", content->valuestring);
            char *my_content = strdup(content->valuestring);
        }


        int sample_rate = espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, NULL, 0);
        if (sample_rate == -1){
            printf("espeak init failed");
            return 1;
        }



        speak(content->valuestring);
        espeak_Terminate();



        cJSON_Delete(root);
    }
    else if(dry){
        printf("No AI was contacted because subrub was ran in dry mode");
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
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) {
        // Out of memory
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;  // Null-terminate
    
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

void speak(const char *text) {
        espeak_Synth(text, 
                 strlen(text) + 1, 
                 0, POS_CHARACTER, 0, 
                 espeakCHARS_UTF8, NULL, NULL);
    espeak_Synchronize();
}