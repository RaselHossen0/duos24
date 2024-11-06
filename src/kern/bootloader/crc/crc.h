#ifndef CRC_H
#define CRC_H

#include <stdint.h>

void crc_init(void);
uint32_t calculate_crc(const uint32_t *data, uint32_t length);

#endif // CRC_H