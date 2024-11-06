#include "bootloader.h"
#include "kstdio.h"
#include "sys_usart.h"
#include "crc.h"
#include "cm4.h"

#define CHUNK_SIZE 128
#define ACK 0x06
#define NACK 0x15
#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xCDEF89AB
#define APPLICATION_ADDRESS 0x08004000
#define FIRMWARE_SIZE 0x40000   // Define firmware size (adjust as needed)
#define __IO volatile           // Define `__IO` as `volatile`
void __sys_init1(void) {
    __init_sys_clock(); // Configure system clock
    __ISB();
    __enable_fpu(); // Enable FPU
    __ISB();
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    __SysTick_init(1000); // Enable SysTick for 1ms
    SerialLin2_init(__CONSOLE, 0);
    Ringbuf_init(__CONSOLE);
    __ISB();
}
Command g_parsed_cmd;  // Global variable to hold the parsed command
// Flash unlocking functions
void unlock_flash(void) {
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
}

void lock_flash(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

// Erase a specific flash sector
void erase_flash_sector(uint32_t sector) {
    unlock_flash();
    FLASH->CR |= FLASH_CR_SER;
    FLASH->CR |= (sector << FLASH_CR_SNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_SER;
    lock_flash();
}

// Program flash with data
void program_flash(uint32_t address, uint32_t *data, uint32_t size) {
    unlock_flash();
    FLASH->CR |= FLASH_CR_PG;

    for (uint32_t i = 0; i < size / 4; i++) {
        *(__IO uint32_t*)address = data[i];
        address += 4;
        while (FLASH->SR & FLASH_SR_BSY);
    }

    FLASH->CR &= ~FLASH_CR_PG;
    lock_flash();
}

// Validate firmware using CRC
int validate_firmware(uint32_t address, uint32_t expected_crc) {
    uint32_t *firmware = (uint32_t *)address;
    uint32_t firmware_crc = calculate_crc(firmware, FIRMWARE_SIZE / 4);
    return (firmware_crc == expected_crc) ? 1 : 0;
}

int is_uart_data_available(UART_HandleTypeDef *uart) {
    if (uart == NULL || uart->pRxBuffPtr == NULL) {
        return 0; // UART handle or buffer is invalid
    }

    // Check if there's data by comparing head and tail pointers
    if (uart->pRxBuffPtr->head != uart->pRxBuffPtr->tail) {
        return 1; // Data available
    } else {
        return 0; // No data available
    }
}
// Bootloader main function
void Bootloader_Main(void) {
    __sys_init1();
    kprintf("Bootloader Initialized.\n");
    ms_delay(1000);

    // Main loop: wait for commands
    while (1) {
        // Get the command by calling parse_command
        parse_command();  // This will update the global g_parsed_cmd

        kprintf("Command received: %x\n", g_parsed_cmd.command_type);

        // Process commands based on the command type
        switch (g_parsed_cmd.command_type) {
            case CMD_START_TRANSFER:
                kprintf("Start file transfer command received.\n");
                receive_file();
                break;

            case CMD_FIRMWARE_SIZE:
                kprintf("Processing firmware size command.\n");
                // handle_firmware_size();
                break;

            case CMD_STATUS_REQUEST:
                kprintf("Status request command received.\n");
                // Additional status processing here
                break;

            case CMD_ABORT_TRANSFER:
                kprintf("Abort transfer command received.\n");
                // Handle transfer abortion
                break;

            default:
                kprintf("Unknown command.\n");
                break;
        }
    }
}
void parse_command(void) {
    uint8_t buffer[4];
    int bytes_read = 0;

    // Wait for the header byte
    while (Uart_read(__CONSOLE) != HEADER_BYTE) {
        // Waiting for the header byte
    }
    buffer[0] = HEADER_BYTE;
    bytes_read = 1;

    // Read remaining bytes: command type, payload size, and checksum
    while (bytes_read < 4) {
        if (is_uart_data_available(__CONSOLE)) {
            buffer[bytes_read++] = Uart_read(__CONSOLE);
        }
    }

    // Assign parsed values to the global cmd structure
    g_parsed_cmd.header = buffer[0];
    g_parsed_cmd.command_type = buffer[1];
    g_parsed_cmd.payload_size = buffer[2];
    g_parsed_cmd.checksum = buffer[3];

    // Debugging prints for confirmation
    kprintf("%x", g_parsed_cmd.header);
    ms_delay(1000);
    kprintf("%x", g_parsed_cmd.command_type);
    ms_delay(1000);
    kprintf("%x", g_parsed_cmd.payload_size);
    ms_delay(1000);
    kprintf("%x", g_parsed_cmd.checksum);
    ms_delay(1000);
}

void receive_file(void) {
    uint8_t buffer[CHUNK_SIZE];
    uint8_t temp_buffer[CHUNK_SIZE];  // Temporary buffer to store received data
    size_t bytes_received = 0;
    uint16_t received_crc, calculated_crc;
    uint8_t chunk_size;
    uint32_t file_size = 0;

    ms_delay(1000);

    // Acknowledge the file size command
    kprintf("%x", ACK);

    // Assuming `g_parsed_cmd.payload_size` holds the file size if sent in `CMD_FIRMWARE_SIZE`
    if (g_parsed_cmd.command_type == CMD_FIRMWARE_SIZE && g_parsed_cmd.payload_size == 4) {
        for (int i = 0; i < 4; i++) {
            file_size |= (Uart_read(__CONSOLE) << (i * 8));
        }
        kprintf("Received file size: %u bytes\n", file_size);
    } else {
        kprintf("Invalid command or unexpected payload size.\n");
        return;
    }

    while (bytes_received < file_size) {
        // Receive CRC (2 bytes)
        uint8_t crc_high = Uart_read(__CONSOLE);
        uint8_t crc_low = Uart_read(__CONSOLE);
        received_crc = (crc_high << 8) | crc_low;

        // Receive chunk size (1 byte)
        chunk_size = Uart_read(__CONSOLE);

        // Check for overflow
        if (bytes_received + chunk_size > file_size) {
            kprintf("Buffer overflow detected. Stopping reception.\n");
            break;
        }

        // Receive data into a temporary buffer
        for (size_t i = 0; i < chunk_size; i++) {
            temp_buffer[i] = Uart_read(__CONSOLE);
        }

        // Copy data from temp_buffer to the actual buffer byte by byte
        for (size_t i = 0; i < chunk_size; i++) {
            buffer[i] = temp_buffer[i];
        }

        // Calculate CRC and verify
        calculated_crc = calculate_crc((uint32_t*)buffer, chunk_size / 4);
        if (calculated_crc == received_crc) {
            Uart_write(ACK, __CONSOLE);
            bytes_received += chunk_size;

            // Program chunk to flash (optional)
            // program_flash(APPLICATION_ADDRESS + bytes_received, (uint32_t*)buffer, chunk_size);
        } else {
            Uart_write(NACK, __CONSOLE);
            kprintf("CRC mismatch. Requesting resend...\n");
        }
    }
}


uint8_t calculate_checksum(const Command *cmd) {
    uint8_t checksum = cmd->header + cmd->command_type + cmd->payload_size;
    for (int i = 0; i < cmd->payload_size; i++) {
        checksum += cmd->payload[i];
    }
    return checksum & 0xFF; // Only keep the lower 8 bits
}

