# SSTV调制程序  

本程序为使用C语言编写的一个SSTV调制模块，将用于悬镜系列卫星的业余无线电活动。

目前支持的模式：  
1. Scottie-DX
2. PD-120

计划支持的模式：  
1. PD-240
2. Robot-36
3. Robot-72

## 编译
```
~ $ gcc sstv.c -o sstv -lm -I./include
```
## 运行
使用命令行参数指定输入文件和调制模式，例如：  
```
./sstv test.png Scottie-DX
```

## 许可证

本项目采用GNU General Public License v3.0许可证。
