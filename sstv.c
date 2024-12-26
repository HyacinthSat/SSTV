/* 

This C program is used to modulate SSTV audio.
It will be used for amateur radio activities with the XuanJing series satellites.

Version: 1.0   Date: December 26, 2024

Developer & Acknowledgments:
    BG7ZDQ - Initial program
    BI4PYM - Enhancements and fixes
    N7CXI  - Document: "Proposal for SSTV Mode Specifications"

License: GNU General Public License v3.0

*/

#define STB_IMAGE_IMPLEMENTATION          // stb预处理器
#define STB_IMAGE_RESIZE_IMPLEMENTATION   // stb预处理器 
#define SAMPLE_RATE 6000                  // 采样率
#define PI 3.14159265358979323846         // 圆周率
#define COLOR_FREQ_MULT 3.1372549         // 颜色频率乘数，用于转换RGB值到频率(0-255映射到1500~2300)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "include/stb_image.h"

// 声明全局变量
FILE *file;                   // 用于存储音频数据的文件指针
uint32_t total_samples;       // 总采样数
const unsigned char *pixels;  // 图像像素数据
int width;                    // 图像宽度
double olderdata;             // 前一个幅度，用于连续相位
double oldercos;              // 前一个COS，用于连续相位
double delta_lenth = 0;       // 采样率精度补偿

// WAV 文件头结构体，用于存储 WAV 文件格式的头部信息
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

// 生成 WAV 文件头
void write_wav_header(uint32_t data_size) {
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

// 函数：判断正负号
int sign(double num) {
    if (num >= 0) {
        return 1;
    } else if (num < 0) {
        return -1;
    } else {
        return 0;
    }
}

// 函数：生成并写入指定频率和持续时间和初始相位的正弦波音频
void write_tone(double frequency, double duration_ms) {
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
}

// 函数：计算像素在某一颜色通道的强度
double rgb_read_pixel(const char *color, int x, int y) {
    int index = (y * width + x) * 3;

    if (strcmp(color, "r") == 0) {
        return (double)pixels[index];
    } else if (strcmp(color, "g") == 0) {
        return (double)pixels[index + 1];
    } else if (strcmp(color, "b") == 0) {
        return (double)pixels[index + 2];
    } else {
        double R = (double)pixels[index];
        double G = (double)pixels[index + 1];
        double B = (double)pixels[index + 2];

        if (strcmp(color, "y") == 0) {
            return 16.0 + (.003906 * ((65.738 * R) + (129.057 * G) + (25.064 * B)));
        } else if (strcmp(color, "ry") == 0) {
            return 128.0 + (.003906 * ((112.439 * R) + (-94.154 * G) + (-18.285 * B)));
        } else if (strcmp(color, "by") == 0) {
            return 128.0 + (.003906 * ((-37.945 * R) + (-74.494 * G) + (112.439 * B)));
        } else {
	        printf("错误的颜色通道，请检查代码\n");
	        exit(-1);
	        return -1;
	    }
    }
}

// 函数：调制 VIS 校准头
void generate_vis(const char *vis_code) {
    
    // CW 呼号 + 快速识别前导 + VIS 码引导音与起始音部分
    struct { double frequency; int duration_ms; } tones[] = {

        // 'D' -> -..
        {1900, 150}, {0, 50}, {1900, 50}, {0, 50}, {1900, 50}, {0, 150},
        // 'E' -> .
        {1900, 50}, {0, 350},
        // 'B' -> -...
        {1900, 150}, {0, 50}, {1900, 50}, {0, 50}, {1900, 50}, {0, 50}, {1900, 50}, {0, 150},
        // 'G' -> --.
        {1900, 150}, {0, 50}, {1900, 150}, {0, 50}, {1900, 50}, {0, 150},
        // '7' -> --...
        {1900, 150}, {0, 50}, {1900, 150}, {0, 50}, {1900, 50}, {0, 50}, {1900, 50}, {0, 50}, {1900, 50}, {0, 150},
        // 'Z' -> --..
        {1900, 150}, {0, 50}, {1900, 150}, {0, 50}, {1900, 50}, {0, 50}, {1900, 50}, {0, 150},
        // 'D' -> -..
        {1900, 150}, {0, 50}, {1900, 50}, {0, 50}, {1900, 50}, {0, 150},
        // 'Q' -> --.-
        {1900, 150}, {0, 50}, {1900, 150}, {0, 50}, {1900, 50}, {0, 50}, {1900, 150}, {0, 350},
        // 前导音
        {1900, 100},{1500, 100},{1900, 100},{1500, 100},{2300, 100},{1500, 100},{2300, 100},{1500, 100},
        // VIS 码引导音
        {1900, 300}, {1200, 10}, {1900, 300}, {1200, 30}

    };
    for (int i = 0; i < 66; i++) {
        write_tone(tones[i].frequency, tones[i].duration_ms);
    }

    // VIS 码 7 位标识数据位部分，注意 VIS 码为小端序
    for (int i = 0; i < 7; i++) {
        double frequency = vis_code[6-i] == '1' ? 1100 : 1300;
        write_tone(frequency, 30);
    }

    // 偶校验位部分
    int parity = 0;
    for (int i = 0; i < 7; i++) {
        if (vis_code[i] == '1') {
            parity++;
        }
    }
    int parity_bit = (parity % 2 == 0) ? 0 : 1;
    write_tone((parity_bit == 0) ? 1300 : 1100, 30);

    // 结束位
    write_tone(1200, 30);
}

// 函数：调制结束音
void generate_end() {
    
    struct { double frequency; int duration_ms; } tones[] = {
        {1500, 500},{1900, 100},{1500, 100},{1900, 100},{1500, 100}
    };
    for (int i = 0; i < 5; i++) {
        write_tone(tones[i].frequency, tones[i].duration_ms);
    }
}

// 函数：调制 Scottie-DX 图像
void generate_scottie_dx() {
    
    // 起始同步脉冲，仅第一行
    write_tone(1200, 9);

    // 图像数据部分
    for(int line = 0; line < 256; line++) {
        // 分离脉冲
        write_tone(1500, 1.5);

        // 绿色扫描
        for(int x = 0; x < 320; x++) {
            write_tone(1500 + rgb_read_pixel("g",x,line)*COLOR_FREQ_MULT, 1.08);
        }

        // 分离脉冲
        write_tone(1500, 1.5);

        // 蓝色扫描
        for(int x = 0; x < 320; x++) {
            write_tone(1500 + rgb_read_pixel("b",x,line)*COLOR_FREQ_MULT, 1.08);
        }

        // 同步脉冲与同步沿
        write_tone(1200, 9);
        write_tone(1500, 1.5);

        // 红色扫描
        for(int x = 0; x < 320; x++) {
            write_tone(1500 + rgb_read_pixel("r",x,line)*COLOR_FREQ_MULT, 1.08);
        }
    }
}

// 函数：调制 PD-120 图像
void generate_pd_120() {

    for (int line = 0; line < 496; line++) {

        // PD 模式一次扫描两行，由偶数行开始
        if (line % 2 == 0) {

            // 长同步脉冲
            write_tone(1200, 20);
            // Porch 脉冲
            write_tone(1500, 2.08);

            // 偶数行亮度扫描
            for(int x = 0; x < 640; x++) {
                write_tone(1500 + rgb_read_pixel("y",x,line)*COLOR_FREQ_MULT, 0.19);
            }

            // 两行RY均值扫描
            for(int x = 0; x < 640; x++) {
                write_tone(1500 + (rgb_read_pixel("ry",x,line) + rgb_read_pixel("ry",x,line+1)) / 2 *COLOR_FREQ_MULT, 0.19);
            }

            // 两行BY均值扫描
            for(int x = 0; x < 640; x++) {
                write_tone(1500 + (rgb_read_pixel("by",x,line) + rgb_read_pixel("by",x,line+1)) / 2 *COLOR_FREQ_MULT, 0.19);
            }

            // 奇数行亮度扫描
            for(int x = 0; x < 640; x++) {
                write_tone(1500 + rgb_read_pixel("y",x,line+1)*COLOR_FREQ_MULT, 0.19);
            }
        }
    }
}

// 函数：调制 Robot-36 图像
void generate_Robot_36() {

    for (int line = 0; line < 240; line++) {

        // 同步脉冲
        write_tone(1200, 9.0);
        // Porch 脉冲
        write_tone(1500, 3.0);

        if (line % 2 == 0) {

            // 偶数行亮度扫描
            for(int x = 0; x < 320; x++) {
                write_tone(1500 + rgb_read_pixel("y",x,line)*COLOR_FREQ_MULT, 0.275);
            }

            //偶数分离脉冲
            write_tone(1500, 4.5);
            // Porch 脉冲
            write_tone(1900, 1.5);

            // 两行RY均值扫描
            for(int x = 0; x < 320; x++) {
                write_tone(1500 + (rgb_read_pixel("ry",x,line) + rgb_read_pixel("ry",x,line+1)) / 2 *COLOR_FREQ_MULT, 0.1375);
            }
        } else {

            // 奇数行亮度扫描
            for(int x = 0; x < 320; x++) {
                write_tone(1500 + rgb_read_pixel("y",x,line)*COLOR_FREQ_MULT, 0.275);
            }

            //奇数分离脉冲
            write_tone(2300, 4.5);
            // Porch 脉冲
            write_tone(1900, 1.5);

            // 两行bY均值扫描
            for(int x = 0; x < 320; x++) {
                write_tone(1500 + (rgb_read_pixel("by",x,line) + rgb_read_pixel("by",x,line+1)) / 2 *COLOR_FREQ_MULT, 0.1375);
            }
        }
    }
}

// 主调制生成函数
int generate(const char *wav_filename, const char *model) {
    
    // 文件写入初始化
    total_samples = 0;// 重置样本计数
    file = fopen(wav_filename, "wb");
    if (!file) { perror("无法打开文件"); return 1; }
    write_wav_header(0);// 写入空的 WAV 头

    // 按模式选择 VIS 码，调制图像内容
    if (strcmp(model, "Scottie-DX") == 0){
        generate_vis("1001100");
        generate_scottie_dx();
        generate_end();
    } else if (strcmp(model, "PD-120") == 0){
        generate_vis("1011111");
        generate_pd_120();
        generate_end();
    } else if (strcmp(model, "Robot-36") == 0){
        generate_vis("0001000");
        generate_Robot_36();
        generate_end();
    }else {
	    printf("错误的调制模式，请使用 .\\sstv --help 获取帮助。\n");
	    exit(-1);
    }

    // 计算数据大小并写入文件头
    uint32_t data_size = total_samples * sizeof(short);
    write_wav_header(data_size);
    fclose(file);
    printf("End.\n");
    return 0;
}

int main(int argc, char *argv[]) {

    // 获取程序开始时间
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int height, channels;
    const char *image_filename;
    const char *model;
    image_filename = argv[1];
    model = argv[2];

    // 命令行提示
    if (argc != 3 || strcmp(argv[1], "--help") == 0) {
        printf("用法: .\\sstv.exe <image_filename> <SSTV model>\n");
        printf("支持的SSTV模式:\n1.Scottie-DX\n2.PD-120\n3.Robot-36\n");
        return 1;
    }

    // 读取图像
    pixels = stbi_load(image_filename, &width, &height, &channels, 3);
    if (!pixels) {
        fprintf(stderr, "无法加载图像文件。\n");
        return -1;
    }

    // 处理 WAV 文件名
    char wav_filename[256];
    snprintf(wav_filename, sizeof(wav_filename), "%s.wav", strtok(strdup(image_filename), "."));

    // 开始调制
    int result = generate(wav_filename, model);

    // 释放图像内存
    stbi_image_free((unsigned char*)pixels);

    // 计算并输出程序运行时间
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    printf("程序运行时间: %lf 秒\n", elapsed_time);

    return result;
}