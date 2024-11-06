import serial
import zlib
import time
import os
import argparse

# Command Definitions
HEADER_BYTE = 0xA5
CMD_START_TRANSFER = 0x01
CMD_FIRMWARE_SIZE = 0x02
ACK = 0x06
NACK = 0x15
CHUNK_SIZE = 128
MAX_RETRIES = 25

def calculate_crc(chunk):
    """Calculate CRC-32 for the given chunk."""
    return zlib.crc32(chunk) & 0xFFFFFFFF  # Mask to get the lower 32-bits

def send_command(ser, command_type, payload=b''):
    """Send a structured command packet with a header, command type, payload size, and payload."""
    packet = bytes([HEADER_BYTE, command_type, len(payload)]) + payload
    checksum = sum(packet) & 0xFF
    packet += bytes([checksum])
    ser.write(packet)
    print(f"Sent command: {packet.hex()}")

def read_from_serial(ser):
    """Continuously read lines from the serial port and display them."""
    messages = []
    try:
        while ser.in_waiting:  # Continue reading if there's data in the buffer
            line = ser.readline().strip()  # Read a full line and remove newline/whitespace
            if line:
                try:
                    decoded_line = line.decode('ascii')
                    print(f"Received: {decoded_line}")
                    messages.append(decoded_line)
                except UnicodeDecodeError:
                    # Display hex representation if it's non-ASCII data
                    hex_repr = line.hex()
                    print(f"Received (hex): {hex_repr}")
                    messages.append(hex_repr)
        return messages

    except serial.SerialException as e:
        print(f"Serial port error: {e}")
        return None
    except Exception as e:
        print(f"Unexpected error: {e}")
        return None
def wait_for_ack(ser, expected_ack, max_attempts=25, delay=0.5):
    """Wait for the expected ACK from the serial port."""
    attempts = 0
    while attempts < max_attempts:
        messages = read_from_serial(ser)  # Get all available messages
        if messages:
            for message in messages:
                print(f"Checking message: {message}")
                if message == expected_ack:
                    print("ACK received.")
                    return True  # ACK received, exit the function
        attempts += 1
        time.sleep(delay)  # Wait a bit before retrying
    print("Failed to receive ACK within the maximum attempts.")
    return False  # ACK not received within the maximum attempts
def send_file(file_name, serial_port):
    if not os.path.isfile(file_name):
        print(f"Error: File '{file_name}' not found.")
        return

    try:
        with open(file_name, 'rb') as f:
            data = f.read()
    except IOError as e:
        print(f"Error reading file '{file_name}': {e}")
        return

    try:
        with serial.Serial(serial_port, 115200, timeout=1) as ser:
            # Send Start Transfer command
            ACK_BYTE = '6'  # or if in ASCII: ACK_BYTE = '\x06'

            # Usage in send_file
            send_command(ser, CMD_START_TRANSFER)
            if not wait_for_ack(ser, ACK_BYTE):
                print("Failed to receive ACK for start transfer command.")
                return

            # Send firmware size as a command
            file_size = len(data)
            print(f"File size: {file_size} bytes")
            send_command(ser, CMD_FIRMWARE_SIZE, file_size.to_bytes(4, 'little'))
            retries = 0
            while retries < MAX_RETRIES:
                ack = read_from_serial(ser)
                if ack == bytes([ACK]):
                    print("ACK received for file size.")
                    break
                retries += 1
                time.sleep(0.5)
            if retries == MAX_RETRIES:
                print("Failed to receive ACK for file size. Aborting transfer.")
                return

            offset = 0
            total_size = len(data)

            print(f"Starting file transfer of '{file_name}', size: {total_size} bytes")

            while offset < total_size:
                chunk = data[offset:offset + CHUNK_SIZE]
                chunk_size = len(chunk)
                crc = calculate_crc(chunk)

                payload = crc.to_bytes(4, 'big') + chunk_size.to_bytes(1, 'big') + chunk
                send_command(ser, CMD_START_TRANSFER, payload)

                ack = read_from_serial(ser)
                if ack == bytes([ACK]):
                    offset += chunk_size
                    retries = 0  # Reset retries on successful transmission
                    print(f"Chunk {offset // CHUNK_SIZE} acknowledged.")
                elif ack == bytes([NACK]):
                    print(f"Chunk {offset // CHUNK_SIZE} not acknowledged. Retrying...")
                    retries += 1
                    if retries > MAX_RETRIES:
                        retry_input = input("Max retries exceeded. Do you want to try sending the file again? (y/n): ")
                        if retry_input.lower() == 'y':
                            offset = 0  # Restart the file transfer from the beginning
                            retries = 0
                            continue
                        else:
                            print("Transfer aborted by user.")
                            return
                    time.sleep(0.5)
                else:
                    print("No response or unexpected response received. Retrying...")
                    retries += 1
                    if retries > MAX_RETRIES:
                        retry_input = input("Max retries exceeded. Do you want to try sending the file again? (y/n): ")
                        if retry_input.lower() == 'y':
                            offset = 0
                            retries = 0
                            continue
                        else:
                            print("Transfer aborted by user.")
                            return
                    time.sleep(0.5)

            print("File transfer completed successfully.")
    except serial.SerialException as e:
        print(f"Serial port error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")

def main():
    parser = argparse.ArgumentParser(description='File transfer over serial.')
    parser.add_argument('port', type=str, help='The serial port to use')
    args = parser.parse_args()

    while True:
        user_input = input("Press '1' to send file, '2' to stop: ")
        if user_input == '1':
            send_file("src/compile/target/duos", args.port)
        elif user_input == '2':
            print("Stopping file transfer.")
            break
        else:
            print("Invalid input. Please press '1' to send file or '2' to stop.")

if __name__ == '__main__':
    main()