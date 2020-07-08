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
uint32_t int8_to_int32(const std::vector<uint8_t> &v, int i);


#endif //