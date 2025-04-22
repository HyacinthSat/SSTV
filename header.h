/*
This C program modulates audio for SSTV (Slow-Scan Television) transmission.
It serves as a reference for developing future image transmission protocols onboard HyacinthSat.

Part 3: Header File  
Version: 0.0.2    Date: April 22, 2025

Developer & Acknowledgments:
    BG7ZDQ - Initial implementation
    BI4PYM - Protocol refinements and code improvements
    N7CXI  - Reference: "Proposal for SSTV Mode Specifications"

License: MIT License
*/


#ifndef HEADER_H
#define HEADER_H

// 声明程序全局函数
int WAV_Initialization();
int WAV_Finalization();
int WAV_Write(double, double);

#endif