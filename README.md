# Subrub

## Overview

Subrub is a user configurable AI assistant made in C that works on most modern operating systems. It uses Miniaudio for the recording and PocketSphinx for the transcription. It was designed using [Hack Club AI](https://ai.hackclub.com/proxy/v1/chat/completions) for the AI testing however the user can provide any URL. The LLM model can also be configured in the [configuration file](https://github.com/bowlofneighs/subrub/blob/main/documentation.md#configuration). Subrub uses LibCurl for communication with the URL and cJSON to parse the returned JSON. Then it uses espeak-ng to turn the returned text into understandable albeit robotic speech.

## Installation

You can download precompiled binaries for MacOS Linux and Windows from the releases page.

## Motives

I was missing a no nonsense CLI for quick questions that would do well with an AI answering them. Subrub was a solution to that problem. I initially wanted to have it send a notification but to do that it would have been very difficult to keep it fully cross-platform. As such it is still only half developed however all the core features still work just fine. One of the things that I was really missing on linux was a way to search the web from the terminal. Subrub is a viable alternative to that.
