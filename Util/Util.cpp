#include "Util.h"

uint8_t *int32_to_int8(uint32_t a){
    uint8_t *temp = new uint8_t[4];
    temp[0] = a >> 24;
    temp[1] = a >> 16;
    temp[2] = a >> 8;
    temp[3] = a;
    return temp;
}

uint32_t int8_to_int32(const uint8_t *v, int i){
    return ((uint32_t) v[i] << 24) + ((uint32_t) v[i+1] << 16) + ((uint32_t) v[i+2] << 8) + ((uint32_t) v[i+3]);
}

uint8_t *int64_to_int8(uint64_t a){
    uint8_t *temp = new uint8_t[8];
    temp[0] = a >> 56;
    temp[1] = a >> 48;
    temp[2] = a >> 40;
    temp[3] = a >> 32;
    temp[4] = a >> 24;
    temp[5] = a >> 16;
    temp[6] = a >> 8;
    temp[7] = a;
    return temp;
}
uint64_t int8_to_int64(const uint8_t *v, int i){
    return ((uint64_t) v[i] << 56) + ((uint64_t) v[i+1] << 48) + ((uint64_t) v[i+2] << 40) + ((uint64_t) v[i+3] << 32) + ((uint64_t) v[i+4] << 24) + ((uint64_t) v[i+5] << 16) + ((uint64_t) v[i+6] << 8) + ((uint64_t) v[i+7]);
}

uint32_t checksum_data(const uint8_t *data, uint32_t beg, uint32_t len){
    return 100341;
}