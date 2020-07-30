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

uint32_t checksum_data(const uint8_t *data, uint32_t beg, uint32_t len){
    return 100341;
}