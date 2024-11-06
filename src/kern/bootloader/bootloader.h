#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include <stdint.h>
#include <sys_init.h>
#include <cm4.h>
#include <sys_clock.h>
#include <sys_usart.h>
#include <serial_lin.h>
#include <sys_gpio.h>
#include <kstdio.h>
#include <debug.h>
#include <timer.h>
#include <UsartRingBuffer.h>
#include <system_config.h>
#include <mcu_info.h>
#include <sys_rtc.h>

// #include "kstdio.h" // Include kstdio.h

int kmain(void);

// Bootloader vector table
extern const uint32_t Bootloader_VectorTable[];

// Application vector table
extern const uint32_t Application_VectorTable[];

// Bootloader main function
void Bootloader_Main(void);


// Function to set the Main Stack Pointer
static inline void __set_MSP(uint32_t topOfMainStack) {
    __asm volatile ("MSR msp, %0" : : "r" (topOfMainStack) : );
}
// Define bootloader constants
#define HEADER_BYTE           0xA5  // Unique header byte for command recognition
#define CHUNK_SIZE            128   // Chunk size for data transfer
#define ACK                   0x06  // Acknowledge byte
#define NACK                  0x15  // Not Acknowledge byte
#define FLASH_KEY1            0x45670123
#define FLASH_KEY2            0xCDEF89AB
#define APPLICATION_ADDRESS   0x08004000
#define FIRMWARE_SIZE         0x40000 // Define firmware size
#define __IO                  volatile // Define `__IO` as `volatile`

// Define command types
#define CMD_START_TRANSFER    0x01  // Start file transfer
#define CMD_FIRMWARE_SIZE     0x02  // Send firmware size
#define CMD_STATUS_REQUEST    0x03  // Request bootloader status
#define CMD_ABORT_TRANSFER    0x04  // Abort file transfer

// Define command structure
typedef struct {
    uint8_t header;           // Header byte (0xA5)
    uint8_t command_type;     // Type of command (e.g., start transfer)
    uint8_t payload_size;     // Size of the payload
    uint8_t payload[CHUNK_SIZE]; // Payload data, if any
    uint8_t checksum;         // Checksum for data integrity
} Command;


#define CMD_START_TRANSFER   0x01
#define CMD_FIRMWARE_SIZE    0x02
#define CMD_STATUS_REQUEST   0x03
#define CMD_ABORT_TRANSFER   0x04

void receive_file(void);
void parse_command(void);

#endif // BOOTLOADER_H