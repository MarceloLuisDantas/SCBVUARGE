#include <stdlib.h>
#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

void encode(char *path) {
    printf("ENCODING\n");

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        printf("Error opening input file\n");
        return;
    }  

    fseek(file, 0, SEEK_END);
    long bytes = ftell(file);
    rewind(file);
    printf("bytes: %ld\n", bytes);

    AVFormatContext *fmt = NULL;
    avformat_alloc_output_context2(&fmt, NULL, NULL, "output.mkv");
    if (!fmt) {
        fprintf(stderr, "Error creating format context.\n");
        return;
    }

    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_FFV1);
    if (!codec) {
        fprintf(stderr, "Codec not found.\n");
        return;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->width = 240;
    codec_ctx->height = 240;
    codec_ctx->pix_fmt = AV_PIX_FMT_GRAY8;
    codec_ctx->time_base = (AVRational){1, 30};
    codec_ctx->framerate = (AVRational){30, 1};

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Error opening codec.\n");
        return;
    }

    AVStream *stream = avformat_new_stream(fmt, NULL);
    avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    avio_open(&fmt->pb, "output.mkv", AVIO_FLAG_WRITE);

    int ret = avformat_write_header(fmt, NULL);
    if (ret < 0) {
        printf("Error writing header.\n");
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Error allocating frame.\n");
        return;
    }

    frame->format = AV_PIX_FMT_RGB24;
    frame->width  = 240;
    frame->height = 240;

    if (av_frame_get_buffer(frame, 0) < 0) {
        fprintf(stderr, "Error allocating frame buffer.\n");
        return;
    }

    long pts = 1;
    int finished = 0;

    while (finished == 0) {
        uint8_t buffer[240 * 240];

        if (pts == 1) {
            size_t i = 0;

            for (; i < 8; i++)
                buffer[i] = (bytes >> (56 - (8*i))) & 0xFF;

            for (size_t j = 0; j < strlen(path); j++) {
                buffer[i] = path[j];
                i++;
            }

            for (; i < sizeof(buffer); i++)
                buffer[i] = 255;

        } else {
            size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);

            if (bytes_read < sizeof(buffer)) {
                for (; bytes_read < sizeof(buffer); bytes_read++)
                    buffer[bytes_read] = 0;

                finished = 1;
            }
        }

        for (int y = 0; y < 240; y++) {
            memcpy(
                frame->data[0] + y * frame->linesize[0],
                buffer + y * frame->width,
                frame->width
            );
        }

        frame->pts = pts;
        pts++;

        ret = avcodec_send_frame(codec_ctx, frame);
        if (ret < 0) {
            printf("Error sending frame.\n");
            return;
        }

        AVPacket *pkt = av_packet_alloc();

        while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
            av_interleaved_write_frame(fmt, pkt);
            av_packet_unref(pkt);
        }
    }

    avcodec_send_frame(codec_ctx, NULL);

    AVPacket *pkt = av_packet_alloc();

    while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
        av_interleaved_write_frame(fmt, pkt);
        av_packet_unref(pkt);
    }

    av_write_trailer(fmt);
    avio_close(fmt->pb);
    avcodec_free_context(&codec_ctx);
    avformat_free_context(fmt);

    fclose(file);
}

void decode(char *path) {
    AVFormatContext *fmt_ctx = NULL;

    if (avformat_open_input(&fmt_ctx, path, NULL, NULL) < 0) {
        printf("Error opening video.\n");
        return;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("Error reading stream info.\n");
        return;
    }

    int video_stream = -1;

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            break;
        }
    }

    if (video_stream == -1) {
        printf("No video stream found.\n");
        return;
    }

    const AVCodec *codec =
        avcodec_find_decoder(fmt_ctx->streams[video_stream]->codecpar->codec_id);

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);

    avcodec_parameters_to_context(codec_ctx,
                                  fmt_ctx->streams[video_stream]->codecpar);

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        printf("Error opening decoder.\n");
        return;
    }

    AVPacket *packet = av_packet_alloc();
    AVFrame  *frame  = av_frame_alloc();

    FILE *output = NULL;
    int frame_index = 0;

    while (av_read_frame(fmt_ctx, packet) >= 0) {

        if (packet->stream_index != video_stream) {
            av_packet_unref(packet);
            continue;
        }

        int ret = avcodec_send_packet(codec_ctx, packet);
        av_packet_unref(packet);

        if (ret < 0) {
            continue;
        }

        while (avcodec_receive_frame(codec_ctx, frame) == 0) {

            int width  = frame->width;
            int height = frame->height;

            uint8_t *buffer = malloc(width * height);

            if (!buffer) {
                printf("Failed to allocate buffer.\n");
                return;
            }

            for (int y = 0; y < height; y++) {
                uint8_t *line = frame->data[0] + y * frame->linesize[0];
                memcpy(buffer + y * width, line, width);
            }

            if (frame_index == 0) {

                long bytes = 0;

                for (int i = 0; i < 8; i++) {
                    bytes |= ((long)buffer[i]) << (56 - i * 8);
                }

                printf("Total bytes: %ld\n", bytes);

                int start = 8;
                int i = start;

                while (i < width * height && buffer[i] != 255) {
                    i++;
                }

                int name_len = i - start;

                char original_name[name_len + 2];
                original_name[0] = '_';

                for (int j = 1; j < name_len + 1; j++) {
                    original_name[j] = buffer[start + j - 1];
                }

                original_name[name_len + 1] = '\0';

                printf("File name: %s\n", original_name);

                output = fopen(original_name, "wb");

                if (!output) {
                    printf("Error creating output file.\n");
                    free(buffer);
                    return;
                }

                frame_index = 1;
            }
            else {
                if (output) {
                    fwrite(buffer, 1, width * height, output);
                }
            }

            free(buffer);
        }
    }

    avcodec_send_packet(codec_ctx, NULL);

    while (avcodec_receive_frame(codec_ctx, frame) == 0) {

        int width  = frame->width;
        int height = frame->height;

        uint8_t *buffer = malloc(width * height);

        for (int y = 0; y < height; y++) {
            uint8_t *line = frame->data[0] + y * frame->linesize[0];
            memcpy(buffer + y * width, line, width);
        }

        if (output) {
            fwrite(buffer, 1, width * height, output);
        }

        free(buffer);
    }

    if (output) {
        fclose(output);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("USAGE: scbvuarge [encode/decode] [FILE]\n");
        return 0;
    }

    char *path = argv[2];
    char *mode = argv[1];

    if (strcmp(mode, "encode") == 0) {
        encode(path);
    } else if (strcmp(mode, "decode") == 0) {
        decode(path);
    } else {
        printf("USAGE: scbvuarge [encode/decode] [FILE]\n");
    }

    return 0;
}