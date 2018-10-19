#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//ref:https://github.com/mackron/dr_libs/blob/master/dr_wav.h
#define DR_WAV_IMPLEMENTATION

#include "timing.h"
#include "dr_wav.h"
#include "noise_suppression.h"

#ifndef nullptr
#define nullptr 0
#endif

#ifndef MIN
#define MIN(A, B)        ((A) < (B) ? (A) : (B))
#endif

//写wav文件
void wavWrite_int16(char *filename, int16_t *buffer, size_t sampleRate, unsigned int channels, size_t totalSampleCount) {
    drwav_data_format format = {0};
    format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
    format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
    format.channels = channels;
    format.sampleRate = (drwav_uint32) sampleRate;
    format.bitsPerSample = 16;
    drwav *pWav = drwav_open_file_write(filename, &format);
    if (pWav) {
        drwav_uint64 samplesWritten = drwav_write(pWav, totalSampleCount, buffer);
        drwav_uninit(pWav);
        if (samplesWritten != totalSampleCount) {
            fprintf(stderr, "ERROR\n");
            exit(1);
        }
    }
}

//读取wav文件
int16_t *wavRead_int16(char *filename, uint32_t *sampleRate, unsigned int* channels, uint64_t *totalSampleCount) {
    /*unsigned int channels;*/
    int16_t *buffer = drwav_open_and_read_file_s16(filename, channels, sampleRate, totalSampleCount);
    if (buffer == nullptr) {
        printf("ERROR.");
    }
    return buffer;
}

//分割路径函数
void splitpath(const char *path, char *drv, char *dir, char *name, char *ext) {
    const char *end;
    const char *p;
    const char *s;
    if (path[0] && path[1] == ':') {
        if (drv) {
            *drv++ = *path++;
            *drv++ = *path++;
            *drv = '\0';
        }
    } else if (drv)
        *drv = '\0';
    for (end = path; *end && *end != ':';)
        end++;
    for (p = end; p > path && *--p != '\\' && *p != '/';)
        if (*p == '.') {
            end = p;
            break;
        }
    if (ext)
        for (s = end; (*ext = *s++);)
            ext++;
    for (p = end; p > path;)
        if (*--p == '\\' || *p == '/') {
            p++;
            break;
        }
    if (name) {
        for (s = p; s < end;)
            *name++ = *s++;
        *name = '\0';
    }
    if (dir) {
        for (s = path; s < p;)
            *dir++ = *s++;
        *dir = '\0';
    }
}

enum nsLevel {
    kLow,
    kModerate,
    kHigh,
    kVeryHigh
};


int nsProcess(int16_t *buffer, uint32_t sampleRate, int samplesCount, enum nsLevel level) {
    if (buffer == nullptr) return -1;
    if (samplesCount == 0) return -1;
    size_t samples = MIN(160, sampleRate / 100);
    if (samples == 0) return -1;
    uint32_t num_bands = 1;
    int16_t *input = buffer;
    size_t nTotal = (samplesCount / samples);
    NsHandle *nsHandle = WebRtcNs_Create();
    int status = WebRtcNs_Init(nsHandle, sampleRate);
    if (status != 0) {
        printf("WebRtcNs_Init fail\n");
        return -1;
    }
    status = WebRtcNs_set_policy(nsHandle, level);
    if (status != 0) {
        printf("WebRtcNs_set_policy fail\n");
        return -1;
    }
    for (int i = 0; i < nTotal; i++) {
        int16_t *nsIn[1] = {input};   //ns input[band][data]
        int16_t *nsOut[1] = {input};  //ns output[band][data]
        WebRtcNs_Analyze(nsHandle, nsIn[0]);
        WebRtcNs_Process(nsHandle, (const int16_t *const *) nsIn, num_bands, nsOut);
        input += samples;
    }
    WebRtcNs_Free(nsHandle);

    return 1;
}

void noise_suppression(char *in_file, char *out_file) {
    //音频采样率
    uint32_t sampleRate = 0;
    //总音频采样数
    uint64_t inSampleCount = 0;
	unsigned int channels = 0;
    int16_t *inBuffer = wavRead_int16(in_file, &sampleRate, &channels, &inSampleCount);

    //如果加载成功
    if (inBuffer != nullptr) {
        double startTime = now();
        nsProcess(inBuffer, sampleRate, inSampleCount, kModerate);
        double time_interval = calcElapsed(startTime, now());
        printf("time interval: %d ms\n ", (int) (time_interval * 1000));
        wavWrite_int16(out_file, inBuffer, sampleRate, channels, inSampleCount);
        free(inBuffer);
    }
}

// int main(int argc, char *argv[]) {
    // printf("WebRtc Noise Suppression\n");
    // printf("blog:http://cpuimage.cnblogs.com/\n");
    // if (argc < 2)
        // return -1;
    // char *in_file = argv[1];
    // char drive[3];
    // char dir[256];
    // char fname[256];
    // char ext[256];
    // char out_file[1024];
    // splitpath(in_file, drive, dir, fname, ext);
    // sprintf(out_file, "%s%s%s_out%s", drive, dir, fname, ext);
    // noise_suppression(in_file, out_file);

    // printf("press any key to exit. \n");
    // getchar();
    // return 0;
// }
