#include <stdio.h>
#include <stdint.h>
#include "can_tool.h"

// 从给定的数据中提取指定长度的位数据 (Intel)
uint64_t get_intel_data_from(const uint8_t *data, const uint8_t startByte, const uint16_t startBit, const uint16_t bitLength)
{
    uint8_t numBytes = 0;
    uint64_t bitValue = 0;
    uint64_t mask = 0;
    uint64_t ret = 0;

    // 计算需要读取的字节数
    numBytes = (startBit + (bitLength - 1)) / 8 + 1;
    for (uint8_t i = startByte; i < startByte + numBytes; i++)
    {
        // uint8_t tmp = data[i];
        // printf("data[%d]: %02x\n", i, tmp);
        bitValue += (data[i] << ((i - startByte) * 8));
    }

    // 移位以对齐起始位
    bitValue = bitValue >> startBit;
    // 创建掩码以提取所需长度的位
    mask = (1UL << bitLength) - 1;
    ret = (bitValue & mask);
    // printf("%02lx\n", mask);
    // printf("[%s]: value  = %02lx\n", __FUNCTION__, ret);
    return ret;
}

void set_intel_data_from(uint8_t *data, uint64_t val, const uint8_t startByte, const uint16_t startBit, const uint16_t bitLength)
{
    uint64_t bitStart = 0;
    uint64_t mask = 0;
    uint8_t numBytes = 0;

    // 计算需要修改的字节数
    numBytes = (startBit + (bitLength - 1)) / 8 + 1;
    // 计算起始位的总位数
    bitStart = startByte * 8 + startBit;
    // 创建一个掩码，用于提取值的低位部分
    mask = (1UL << bitLength) - 1;
    // 使用掩码将值的低位部分提取出来
    val &= mask;
    // 将值左移到正确的位置
    val = val << bitStart;

    // 创建一个掩码，用于清除目标区域的位
    uint64_t clearMask = mask << bitStart;

    // 遍历需要修改的字节，清除目标区域的位
    for (uint8_t i = startByte; i < startByte + numBytes; i++) {
        data[i] &= ~(clearMask >> (i * 8));
    }

    // 遍历需要修改的字节，将值写入数据数组中
    for (uint8_t i = startByte; i < startByte + numBytes; i++) {
        data[i] |= (val >> (i * 8));
        // printf("%s:data[%d]: %02x\n", __func__, i, data[i]);
    }
}

// 从给定的数据中提取指定长度的位数据 (Motorola Backward)
uint64_t get_motorola_backward_data_from(const uint8_t *data, uint8_t dlc, const uint8_t startByte, const uint16_t startBit, const uint16_t bitLength)
{
    uint8_t numBytes = 0;
    uint64_t bitValue = 0;
    uint64_t mask = 0;
    uint64_t ret = 0;
    int k = 0;

    // 计算需要读取的字节数
    numBytes = (startBit % 8 + (bitLength - 1)) / 8 + 1;
    k = numBytes;

    for(uint8_t i = startByte + numBytes - 1; i >= startByte ;i--) {
        uint8_t tmp = data[dlc-i-1];
        if(k > 1) {
            bitValue += (tmp << (((i - startByte)) * 8));
            k--;
        }
        else {
            bitValue += tmp;
        }
    }

    // 移位以对齐起始位
    bitValue = bitValue >> (startBit % 8);
    // 创建掩码以提取所需长度的位
    mask = (1UL << bitLength) - 1;
    ret = (bitValue & mask);
    return ret;
}

void set_motorola_backward_data_from(uint8_t *data, uint64_t val, uint8_t dlc ,const uint8_t startByte, const uint16_t startBit, const uint16_t bitLength)
{
    uint64_t mask = 0;
    uint8_t numBytes = 0;
    int k = 0;

    // 计算需要修改的字节数
    numBytes = (startBit % 8 + (bitLength - 1)) / 8 + 1;
    k = numBytes;

    // 创建一个掩码，用于提取值的低位部分
    mask = (1UL << bitLength) - 1;

    // 使用掩码将值的低位部分提取出来
    val &= mask;

    // 创建一个掩码，用于清除目标区域的位
    uint64_t clearMask = mask << startBit;

    // 遍历需要修改的字节，清除目标区域的位
    for (uint8_t i = startByte; i < startByte + numBytes; i++) {
        data[dlc-i-1] &= ~(clearMask >> (i * 8));
    }

    for(uint8_t i = startByte + numBytes - 1; i >= startByte ;i--) {
        if(k > 1){
            data[dlc-i-1] |= (val >> ((i - startByte) * 8));
            k--;
        }
        else{
            data[dlc-i-1] |= (val << (startBit % 8));
        }
    }
}
