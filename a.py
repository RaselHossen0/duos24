import serial
import time
import os

SERIAL_PORT = '/dev/tty.usbmodem11403'  # Update with your serial port
BAUD_RATE = 115200
OUTPUT_FILE = 'received_duos.bin'

def is_port_available(port):
    try:
        with serial.Serial(port) as ser:
            return True
    except serial.SerialException:
        return False

def receive_file():
    while True:
        if is_port_available(SERIAL_PORT):
            try:
                with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                    with open(OUTPUT_FILE, 'wb') as f:
                        print("Listening for incoming data...")
                        start_time = time.time()
                        total_bytes_received = 0
                        while True:
                            chunk = ser.read(256)
                            if not chunk:
                                break
                            f.write(chunk)
                            total_bytes_received += len(chunk)
                            print(f"Received chunk of size: {len(chunk)}")  # Debugging information
                        end_time = time.time()
                        f.flush()  # Ensure all data is written to the file
                        os.fsync(f.fileno())  # Ensure all data is written to disk
                        if total_bytes_received > 0:
                            duration = end_time - start_time
                            file_size = os.path.getsize(OUTPUT_FILE)
                            print(f"File received and saved as {OUTPUT_FILE}")
                            print(f"Total bytes received: {total_bytes_received}")
                            print(f"File size: {file_size} bytes")
                            print(f"Time taken: {duration:.2f} seconds")
                            return  # Terminate the program after receiving the file
            except serial.SerialException as e:
                print(f"Serial exception: {e}")
                print("Retrying in 5 seconds...")
                time.sleep(5)  # Wait for 5 seconds before retrying
            except Exception as e:
                print(f"Unexpected exception: {e}")
                print("Retrying in 5 seconds...")
                time.sleep(5)  # Wait for 5 seconds before retrying
        else:
            print(f"Serial port {SERIAL_PORT} is not available. Retrying in 5 seconds...")
            time.sleep(5)  # Wait for 5 seconds before retrying
        time.sleep(1)  # Wait for 1 second before listening again

if __name__ == '__main__':
    receive_file()