#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <windows.h>

// 定义一些常量，用于调制产生音频和控制音频文件的参数
#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846

// 声明全局变量
FILE *file;                   // 用于存储音频数据的文件指针
uint32_t total_samples;   // 总采样数

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
        .riff = "RIFF",                        // "RIFF" 文件标识
        .chunk_size = 36 + data_size,          // 文件头与数据部分的总大小
        .wave = "WAVE",                        // "WAVE" 文件标识
        .fmt = "fmt ",                         // "fmt " 格式标识
        .subchunk1_size = 16,                  // 格式块大小为 16 字节
        .audio_format = 1,                     // PCM 格式
        .num_channels = 1,                     // 单声道
        .sample_rate = SAMPLE_RATE,            // 设置采样率
        .byte_rate = SAMPLE_RATE * 1 * 16 / 8, // 每秒字节数
        .block_align = 1 * 16 / 8,             // 块对齐
        .bits_per_sample = 16,                 // 每个样本 16 位
        .data = "data",                        // 音频数据标识
        .subchunk2_size = data_size            // 音频数据大小
    };
    fseek(file, 0, SEEK_SET);                  // 确保写在文件开头
    fwrite(&header, sizeof(WAVHeader), 1, file);  // 将文件头写入文件
}

// 函数：生成并写入指定频率和持续时间的正弦波音频
void write_tone(double frequency, double duration_ms) {
    uint32_t num_samples = (SAMPLE_RATE * duration_ms) / 1000;
    short buffer[num_samples];
    for (uint32_t i = 0; i < num_samples; ++i) {
        buffer[i] = (short)(32767 * sin(2 * PI * frequency * i / SAMPLE_RATE));
    }
    fwrite(buffer, sizeof(short), num_samples, file);

    // 计算并累加总样本数
    total_samples += num_samples;
}

void sstvmodel(const char *model){
    if(model == "PD120"){

    }
}

// 开始调制
void generate(const char *filename, const char *vis_code, const char *model) {
    total_samples = 0;    // 重置样本计数
    file = fopen(filename, "wb");
    if (!file) { perror("无法打开文件"); return; }

    write_wav_header(0);    // 写入空的 WAV 头

    // 快速识别前导+VIS码引导音与起始音部分
    struct { double frequency; int duration_ms; } tones[] = {
        {1900, 100},{1500, 100},{1900, 100},{1500, 100},{2300, 100},{1500, 100},{2300, 100},{1500, 100},{1900, 300}, {1200, 10}, {1900, 300}, {1200, 30}
    };
    for (int i = 0; i < 12; i++) {
        write_tone(tones[i].frequency, tones[i].duration_ms);
    }

    // VIS 码 7 位标识数据位部分
    for (int i = 0; i < 7; i++) {
        // 注意 VIS 码为小端序，所以从右到左读取(即 6-i )
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

    

    // 计算数据大小并写入文件头
    uint32_t data_size = total_samples * sizeof(short);
    write_wav_header(data_size);
    fclose(file);

    printf("WAV 文件已生成: %s\n", filename);
}

int main() {
    char filename, vis_code, model;
    SetConsoleOutputCP(65001);

    scanf("%c %c", filename, model);

    if(model == "PD120"){
        vis_code = "1011111";
    }else if(model == "Scottie 1"){
        vis_code = "0111100";
    }else if(model == "Scottie 2"){
        vis_code = "0111000";
    }else if(model == "Scottie DX"){
        vis_code = "1001100";
    }else{
        printf("不支持的SSTV模式");
        return 0;
    }

    generate(filename, vis_code, model);

    return 0;
}

/*

所有标准 SSTV 模式在开始前都使用一个独特的数字代码来向接收系统标识该模式。
该代码称为 VIS，即垂直间隔信号代码（Vertical Interval Signal code）。
虽然整个校准头通常被称为“VIS 代码”，但代码本身只是其中的一部分。

VIS码共13位，格式如下：

时长（ms）    频率（hz）     类型
----------------------------------
300           1900          引导音
10            1200          中断
300           1900          引导音
30            1200          VIS起始位
30            bit0          1100Hz=1, 1300Hz=0
30            bit1          ""
30            bit2          ""
30            bit3          ""
30            bit4          ""
30            bit5          ""
30            bit6          ""
30            校验          偶数=1300hz, 奇数=1100hz
30            1200          VIS结束位

注意：
VIS 代码为小端序，调制时应从最低位开始。
校验模式为偶校验，即前7位中1的个数应为偶数。
VIS 代码结束后，不管是什么 SSTV 模式，其信号都应紧跟着开始传输。

-------------------------------------------------------------------

各个调制模式的VIS编码：

Scottie 1:   0111100 (60 d)
Scottie 2:   0111000 (56 d)
Scottie DX:  1001100 (76 d)

*/