# SCBVUARGE
## 🇧🇷 Super conversor de binário em vídeo para uso em ARG e Espionagem
## 🇨🇦 Super Converter of Binary into Video For use in ARG and Spy

## Build and Use - Just work on Linux 64
You will need ffmpeg installed, 
```
$ make build
gcc main.c -o scbvuarge -Wall -Wextra -Werror -g -lavformat -lavcodec -lswresample -lswscale -lavutil -lm

```