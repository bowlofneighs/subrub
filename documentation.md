# Subrub

Subrub is a user configurable AI assistant made in C that works on most modern operating systems[<sup>1</sup>](#notes). It uses Miniaudio for the recording and PocketSphinx for the transcription. It was designed using [Hack Club AI](https://ai.hackclub.com/proxy/v1/chat/completions) for the AI testing however the user can provide any URL[<sup>2</sup>](#notes). The LLM model can also be configured in the [configuration file](#configuration). Subrub uses LibCurl for communication with the URL and cJSON to parse the returned JSON.

## Installation
### Prebuilt Binary
The easiest way to install Subrub is to download the correct binary for your system from the [Releases Page](https://github.com/bowlofneighs/subrub/releases)[<sup>3</sup>](#notes)

### Building From Source
Dependencies:
- gcc
- miniaudio
- pocketsphinx
- cjson
- espeak-ng

You can then build it from source with this command:

```
git clone https://github.com/bowlofneighs/subrub
cd subrub
gcc main.c -o subrub -lm -lcurl -lpocketsphinx -lcjson -lespeak-ng
```


## Configuration

The configuration file follows a common `key = value` syntax.

Comments denoted by a `#` until a newline character. For example \
```
# this would be a comment in your config file because of the leading "#"
however this would no longer be a comment, and the parser would throw an error
```
There are two[<sup>4</sup>](#notes) main sections in the config file, the `AI` section is configured like so:
```
    AI{
        key = value
        # this is the ai section, the "key = value" above is an example option
        # the currently available options are:
        api_key = sk-hc-v1-1234567890abcdefghijklmnopqrstuvwxyz
        # this is where you define the API key

        model = google/gemini-3-flash-preview
        # if you are using ai.hackclub.com then you can see a list of available models there.

        url = https://ai.hackclub.com/proxy/v1/chat/completions
        # this is the curl endpoint that subrub will use, use this URL for ai.hackclub.com
    }

```
A Keyword is a string that if detected by the transcriber, will run a user defined command. Subrub will not send the query to the LLM if a Keyword is detected. They are defined like so:
```
    Keywords{
        sound{
            keyword = "play it" # this is what you say to run the command
            command = "aplay /dev/urandom" # this is the command that is run
        }
        less-sounds{
            keyword = "play it less"
            command = "amixer sset 'Master' 10%-"
        }
    }
```



## Notes

1. MacOS and Windows needs testing, however Linux does work
2. Not sure if it works with other URLs
3. There will likely be no MacOS binary at release because I don't have a way to compile it, if someone could provide me with one that would be great
4. A section to configure the TTS is planned
