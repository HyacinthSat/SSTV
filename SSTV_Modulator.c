/*
This C program modulates audio for SSTV (Slow-Scan Television) transmission.
It serves as a reference for developing future image transmission protocols onboard HyacinthSat.

Part 1: Modulate SSTV
Version: 0.0.2    Date: April 22, 2025

Developer & Acknowledgments:
    BG7ZDQ - Initial implementation
    BI4PYM - Protocol refinements and code improvements
    N7CXI  - Reference: "Proposal for SSTV Mode Specifications"

License: MIT License
*/

// 定义程序内全局常量
#define STB_IMAGE_IMPLEMENTATION          // stb预处理器
#define STB_IMAGE_RESIZE_IMPLEMENTATION   // stb预处理器 
#define COLOR_FREQ_MULT 3.1372549         // 颜色频率乘数，用于转换RGB值到频率(0-255映射到1500~2300)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "stb_image.h"

// 定义程序内全局变量
unsigned char *pixels;        // 图像原始像素数据
char *filename;               // WAV 容器文件名
int channels;                 // 图像通道数
int height;                   // 图像高度
int width;                    // 图像宽度

// 声明内部函数
double Channel_Value(char *, int, int);
int Preprocessing(char *, char *);
int Generate_VIS(char *);
int Generate_End();
int Generate_Scottie_DX();
int Generate_Robot_36();
int Generate_PD_120();


// 程序总入口点
int main(int argc, char *argv[]) {

    // 命令行提示
    if (argc != 4 || strcmp(argv[1], "--help") == 0) {
        printf("用法: ./sstv <Image Filename> <SSTV Model> <Output File>\n");
        printf("例如: ./sstv 'test.jpg' 'Robot-36' 'Output.wav'\n");
        printf("支持的SSTV模式:\n 1.Scottie-DX\n 2.PD-120\n 3.Robot-36\n");
        printf("注意: 确保输入带有连字符的正确的调制模式名。\n");
        return 1;
    }

    filename = argv[3];

    // 调用预处理函数
    return Preprocessing(argv[1], argv[2]);
}

// 预处理函数
int Preprocessing(char *image, char *model) {

    // 读取图像
    pixels = stbi_load(image, &width, &height, &channels, 3);
    if (!pixels) {
        printf("图像文件加载失败。\n");
        return -1;
    }

    // 初始化 WAV 容器
    WAV_Initialization();

    // 按模式选择 VIS 前导码并调用相关函数
    if (strcmp(model, "Scottie-DX") == 0){
        Generate_VIS("1001100");
        Generate_Scottie_DX();
    } else if (strcmp(model, "PD-120") == 0){
        Generate_VIS("1011111");
        Generate_PD_120();
    } else if (strcmp(model, "Robot-36") == 0){
        Generate_VIS("0001000");
        Generate_Robot_36();
    }else {
	    printf("错误的调制模式，请使用 ./sstv --help 获取帮助。\n");
	    return -1;
    }
    
    // 释放 WAV 容器
    WAV_Finalization();

    // 释放图像内存
    stbi_image_free((unsigned char*)pixels);

    return 0;
}

// 调制 VIS 前导头
int Generate_VIS(char *vis_code) {
    
    // 快速识别前导 + VIS 码引导音与起始音部分
    struct {
        double frequency; int duration_ms;
    } tones[] = {
        // 前导音
        {1900, 100},{1500, 100},{1900, 100},{1500, 100},{2300, 100},{1500, 100},{2300, 100},{1500, 100},
        // VIS 码引导音
        {1900, 300}, {1200, 10}, {1900, 300}, {1200, 30}
    };

    for (int i = 0; i < 12; i++) {
        WAV_Write(tones[i].frequency, tones[i].duration_ms);
    }

    // VIS 码 7 位标识数据位部分。VIS 码为小端序
    for (int i = 6; i >= 0; i--) {
        double frequency = vis_code[i] == '1' ? 1100 : 1300;
        WAV_Write(frequency, 30);
    }

    // 偶校验位部分
    int ones = 0;
    for (int i = 0; i < 7; i++) ones += (vis_code[i] == '1');
    WAV_Write((ones % 2 == 0) ? 1300 : 1100, 30);

    // 结束位
    WAV_Write(1200, 30);

    return 0;
}

// 函数：调制结束音
int Generate_End() {
    
    struct {
        double frequency; int duration_ms;
    } tones[] = {
        {1500, 500},{1900, 100},{1500, 100},{1900, 100},{1500, 100}
    };

    for (int i = 0; i < 5; i++) {
        WAV_Write(tones[i].frequency, tones[i].duration_ms);
    }

    return 0;
}

// 计算像素在某一颜色通道的强度
double Channel_Value(char *channel, int x, int y) {
    int index = (y * width + x) * 3;

    // RGB 色彩模式
    if (strcmp(channel, "r") == 0) {
        return (double)pixels[index];
    } else if (strcmp(channel, "g") == 0) {
        return (double)pixels[index + 1];
    } else if (strcmp(channel, "b") == 0) {
        return (double)pixels[index + 2];
    }
    
    // YRYB 色彩模式
    else {
        double R = (double)pixels[index];
        double G = (double)pixels[index + 1];
        double B = (double)pixels[index + 2];

        if (strcmp(channel, "y") == 0) {
            return 16.0 + (.003906 * ((65.738 * R) + (129.057 * G) + (25.064 * B)));
        } else if (strcmp(channel, "ry") == 0) {
            return 128.0 + (.003906 * ((112.439 * R) + (-94.154 * G) + (-18.285 * B)));
        } else if (strcmp(channel, "by") == 0) {
            return 128.0 + (.003906 * ((-37.945 * R) + (-74.494 * G) + (112.439 * B)));
        }
    }
}

// Scottie-DX 模式
int Generate_Scottie_DX() {
    
    // 起始同步脉冲，仅第一行
    WAV_Write(1200, 9);

    // 图像数据部分
    for(int row = 0; row < 256; row++) {
        // 分离脉冲
        WAV_Write(1500, 1.5);

        // 绿色扫描
        for(int col = 0; col < 320; col++) {
            WAV_Write(1500 + Channel_Value("g",col,row)*COLOR_FREQ_MULT, 1.08);
        }

        // 分离脉冲
        WAV_Write(1500, 1.5);

        // 蓝色扫描
        for(int col = 0; col < 320; col++) {
            WAV_Write(1500 + Channel_Value("b",col,row)*COLOR_FREQ_MULT, 1.08);
        }

        // 同步脉冲与同步沿
        WAV_Write(1200, 9);
        WAV_Write(1500, 1.5);

        // 红色扫描
        for(int col = 0; col < 320; col++) {
            WAV_Write(1500 + Channel_Value("r",col,row)*COLOR_FREQ_MULT, 1.08);
        }
    }

    return 0;
}

// PD-120 模式
int Generate_PD_120() {

    // PD-120 模式共扫描 496 行
    for (int row = 0; row < 496; row++) {

        // PD 模式一次扫描两行，由偶数行开始
        if (row % 2 == 0) {

            // 长同步脉冲
            WAV_Write(1200, 20);
            // Porch 脉冲
            WAV_Write(1500, 2.08);

            // 偶数行亮度扫描
            for(int col = 0; col < 640; col++) {
                WAV_Write(1500 + Channel_Value("y",col,row)*COLOR_FREQ_MULT, 0.19);
            }

            // 两行RY均值扫描
            for(int col = 0; col < 640; col++) {
                WAV_Write(1500 + (Channel_Value("ry",col,row) + Channel_Value("ry",col,row+1)) / 2 *COLOR_FREQ_MULT, 0.19);
            }

            // 两行BY均值扫描
            for(int col = 0; col < 640; col++) {
                WAV_Write(1500 + (Channel_Value("by",col,row) + Channel_Value("by",col,row+1)) / 2 *COLOR_FREQ_MULT, 0.19);
            }

            // 奇数行亮度扫描
            for(int col = 0; col < 640; col++) {
                WAV_Write(1500 + Channel_Value("y",col,row+1)*COLOR_FREQ_MULT, 0.19);
            }
        }
    }

    return 0;
}

// Robot-36 模式
int Generate_Robot_36() {

    // Robot-36 模式共扫描 240 行
    for (int row = 0; row < 240; row++) {

        // 同步脉冲
        WAV_Write(1200, 9.0);
        // Porch 脉冲
        WAV_Write(1500, 3.0);

        if (row % 2 == 0) {

            // 偶数行亮度扫描
            for(int col = 0; col < 320; col++) {
                WAV_Write(1500 + Channel_Value("y",col,row)*COLOR_FREQ_MULT, 0.275);
            }

            //偶数分离脉冲
            WAV_Write(1500, 4.5);
            // Porch 脉冲
            WAV_Write(1900, 1.5);

            // 两行RY均值扫描
            for(int col = 0; col < 320; col++) {
                WAV_Write(1500 + (Channel_Value("ry",col,row) + Channel_Value("ry",col,row+1)) / 2 *COLOR_FREQ_MULT, 0.1375);
            }
        } else {

            // 奇数行亮度扫描
            for(int col = 0; col < 320; col++) {
                WAV_Write(1500 + Channel_Value("y",col,row)*COLOR_FREQ_MULT, 0.275);
            }

            //奇数分离脉冲
            WAV_Write(2300, 4.5);
            // Porch 脉冲
            WAV_Write(1900, 1.5);

            // 两行bY均值扫描
            for(int col = 0; col < 320; col++) {
                WAV_Write(1500 + (Channel_Value("by",col,row) + Channel_Value("by",col,row+1)) / 2 *COLOR_FREQ_MULT, 0.1375);
            }
        }
    }

    return 0;
}