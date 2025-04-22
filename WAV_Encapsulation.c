/*
This C program modulates audio for SSTV (Slow-Scan Television) transmission.
It serves as a reference for developing future image transmission protocols onboard HyacinthSat.

Part 2: Encapsulation of the baseband signal into WAV file  
Version: 0.0.2    Date: April 22, 2025

Developer & Acknowledgments:
    BG7ZDQ - Initial implementation
    BI4PYM - Protocol refinements and code improvements
    N7CXI  - Reference: "Proposal for SSTV Mode Specifications"

License: MIT License
*/

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"

// 定义全局常量
#define SAMPLE_RATE 44100                  // WAV文件采样率
#define PI 3.14159265358979323846          // 圆周率

// 获取外部变量
extern char *filename;

// 定义程序内全局变量
FILE *file;                   // 容器的文件指针
uint32_t total_samples;       // 总采样数
double olderdata;             // 前一个幅度，用于连续相位
double oldercos;              // 前一个COS，用于连续相位
double delta_lenth = 0;       // 采样率精度补偿

// 声明程序内函数
int Write_WAV_Header(uint32_t);
int WAV_Initialization();
int sign(double);
int WAV_Write(double, double);
int WAV_Finalization();

// 结构体：用于存储 WAV 文件格式的头部信息
typedef struct {
    char riff[4];
    uint32_t chunk_size;
    char wave[4];
    char fmt[4];
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t subchunk2_size;
} WAVHeader;

// 写入 WAV 文件头
int Write_WAV_Header(uint32_t data_size) {
    WAVHeader header = {
        .riff = "RIFF",
        .chunk_size = 36 + data_size,
        .wave = "WAVE",
        .fmt = "fmt ",
        .subchunk1_size = 16,
        .audio_format = 1,
        .num_channels = 1,
        .sample_rate = SAMPLE_RATE,
        .byte_rate = SAMPLE_RATE * 1 * 16 / 8,
        .block_align = 1 * 16 / 8,
        .bits_per_sample = 16,
        .data = "data",
        .subchunk2_size = data_size
    };
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(WAVHeader), 1, file);
}

// 文件初始化，创建文件并写入文件头
int WAV_Initialization() {
    total_samples = 0;
    file = fopen(filename, "wb");
    if (!file) {
        printf("无法打开文件");
        return -1;
    }
    Write_WAV_Header(0);
    WAV_Write(0, 200);

    return 0;
}

// 判断正负号
int sign(double num) {
    if (num >= 0) {
        return 1;
    } else if (num < 0) {
        return -1;
    } else {
        return 0;
    }
}

// Todo: 拓展为频率、开始时间、持续时长、相位四个参数，以实现在同一时间存入多种频率分量和对相位调制的支持

// 生成并向WAV容器写入指定频率和持续时间的正弦波
int WAV_Write(double frequency, double duration_ms) {
    uint32_t num_samples = SAMPLE_RATE * duration_ms / 1000;
    delta_lenth += SAMPLE_RATE * duration_ms / 1000 - num_samples;
    if (delta_lenth >= 1) {
        num_samples += (int)delta_lenth;
        delta_lenth -= (int)delta_lenth;
    }
    double phi_samples = SAMPLE_RATE * (sign(oldercos) * asin(olderdata) + abs(sign(oldercos) - 1) / 2 * PI);
    short buffer[num_samples];
    for (uint32_t i = 0; i < num_samples; ++i) {
        buffer[i] = (short)(32767 * sin((2 * PI * frequency * i + phi_samples) / SAMPLE_RATE));
    }
    fwrite(buffer, sizeof(short), num_samples, file);
    total_samples += num_samples;
    olderdata = sin((2 * PI * frequency * num_samples + phi_samples) / SAMPLE_RATE);
    oldercos = cos((2 * PI * frequency * num_samples + phi_samples) / SAMPLE_RATE);

    return 0;
}

// 收尾工作，更新数据大小并关闭文件
int WAV_Finalization() {

    WAV_Write(0, 200);

    uint32_t data_size = total_samples * sizeof(short);
    Write_WAV_Header(data_size);
    fclose(file);

    printf("End.\n");
    return 0;
}