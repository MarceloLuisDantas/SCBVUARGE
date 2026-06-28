# SCBVUARGE
### 🇧🇷 Super conversor de binário em vídeo para uso em ARG e Espionagem
### 🇨🇦 Super Converter of Binary into Video For use in ARG and Spy

## Build and Use - Just work on Linux 64
You will need ffmpeg installed, with all the needed libs: lavformat lavcodec lswresample lswscale lavutil. 
It's is easy to install, just take a quick look on google.

```
$ make build
gcc main.c -o scbvuarge -Wall -Wextra -Werror -g -lavformat -lavcodec -lswresample -lswscale -lavutil -lm
$ ./scbvuarge encode dantes.pdf
ENCODING
bytes: 6887319
# ./scbvuarge decode output.mkv
Total bytes: 6887319
File name: _dantes.pdf
# ./scbvuarge decode rick.mkv
Total bytes: 87835922
File name: _video.mp4
```

