#include "Util.h"

uint8_t *int32_to_int8(uint32_t a){
    uint8_t *temp = new uint8_t[4];
    temp[0] = a >> 24;
    temp[1] = a >> 16;
    temp[2] = a >> 8;
    temp[3] = a;
    return temp;
}

uint32_t int8_to_int32(const std::vector<uint8_t> &v, int i){
    assert(i + 3 < v.size());
    return ((uint32_t) v[i] << 24) + ((uint32_t) v[i+1] << 16) + ((uint32_t) v[i+2] << 8) + ((uint32_t) v[i+3]);
}