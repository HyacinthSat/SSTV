# SSTV

本仓库用于示范SSTV的调制，以备后续图像传输方案设计的参考需求。

## 目录  

- SSTV_Modulator.c
- WAV_Encapsulation.c
- test.png

## 功能 
  
支持如下调制模式：
- Scottie-DX
- PD-120  
- Robot-36

将计划支持的模式：  
- PD-50
- PD-90
- PD-240
- Robot-72
- 其他

## 编译  

WAV 版本：
```
gcc SSTV_Modulator.c WAV_Encapsulation.c -o sstv -lm -I./include
```

ALSA 版本目前暂不提供。

## 用法  

使用命令行参数指定输入文件和调制模式。  
用法:  
```
./sstv <'Image Filename'> <'SSTV Model'> <'Output Filename'>
```  

例如:  
```
./sstv "test.jpg" "Robot-36" "Output.wav"
```  

注意: 确保输入带有`-`符号的正确的调制模式名。

## 注意

- **！！没有实现音频滤波器！！**
- 目前仅保证支持 gcc 编译器
- 尚未支持对图像大小的调整

本程序未对输出的音频进行滤波，占用带宽可能过大，谨慎通过SSB模式进行传输！

## 许可证  

本仓库在 MIT 许可证下发布。  
这意味着：

- 任何人均可自由使用、复制、修改本仓库中的内容，包括个人和商业用途。

- 允许以原始或修改后的形式分发本仓库的内容，但须附带原许可证声明，确保后续用户了解其授权条款。

- 本仓库内容按“现状（as-is）”提供，不附带任何明示或暗示的担保，使用者需自行承担风险。

- 仓库贡献者无需对使用本内容造成的任何损失或问题负责。

- 其余未说明但未尽的事项。更多信息请参见：[LICENSE](https://github.com/HyacinthSat/SSTV/blob/main/LICENSE)

**注意**：该许可证条款仅应用于本项目组所著之内容（除特有说明外），在其之外的各类内容均遵循源许可证或版权要求。
