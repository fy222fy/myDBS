#ifndef OB_UTIL
#define OB_UTIL

#include<string>
#include<vector>
#include<cassert>

/**
 * 将32位无符号整数转换为4个8位无符号整数数组
 * a：输入的32位无符号整数
 * return：返回的数组
*/
uint8_t *int32_to_int8(uint32_t a);
/**
 * 将vector中的一段连续4位整数合并成32位无符号整数
 * v：输入的任意长数组
 * i：开始转换的位置
*/
uint32_t int8_to_int32(const uint8_t *v, int i);

uint8_t *int64_to_int8(uint64_t a);
uint64_t int8_to_int64(const uint8_t *v, int i);

//通用的数据校验和计算
uint32_t checksum_data(const uint8_t *data, uint32_t beg, uint32_t len);
#endif //