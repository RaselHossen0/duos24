#include "crc.h"

// Polynomial for CRC-32 (Ethernet, ZIP, etc.)
#define CRC32_POLYNOMIAL 0x04C11DB7

static uint32_t crc_table[256] = {0};

// Initialize CRC table (only needs to be done once)
void crc_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i << 24;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
        crc_table[i] = crc;
    }
}

// Calculate CRC32 for given data
uint32_t calculate_crc(const uint32_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < length; i++) {
        uint8_t byte = (data[i] >> 24) & 0xFF;
        crc = (crc << 8) ^ crc_table[(crc >> 24) ^ byte];
        
        byte = (data[i] >> 16) & 0xFF;
        crc = (crc << 8) ^ crc_table[(crc >> 24) ^ byte];
        
        byte = (data[i] >> 8) & 0xFF;
        crc = (crc << 8) ^ crc_table[(crc >> 24) ^ byte];
        
        byte = data[i] & 0xFF;
        crc = (crc << 8) ^ crc_table[(crc >> 24) ^ byte];
    }

    return ~crc;  // Final XOR for CRC32
}