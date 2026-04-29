import serial
import time
import sys
import socket
import random

# --- CONFIGURE YOUR TARGETS HERE ---
PORT = 'COM11'         # Your ESP8266 Attacker Port
BAUD = 115200
RPI_IP = '10.168.207.209' # Your Raspberry Pi IP
UDP_PORT = 5005

try:
    ser = serial.Serial()
    ser.port = PORT
    ser.baudrate = BAUD
    ser.timeout = 1
    ser.write_timeout = 1
    
    ser.setDTR(False)
    ser.setRTS(False)
    ser.open()
    
    ser.setDTR(True)
    time.sleep(0.05)
    ser.setDTR(False)
    
    print("[*] Waiting for ESP8266 to boot...")
    time.sleep(2) 
    ser.reset_input_buffer()
    ser.reset_output_buffer()

except Exception as e:
    print(f"[FATAL] Error opening port {PORT}: {e}")
    sys.exit()

udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("\n" + "="*60)
print(" 3-VECTOR ML CHALLENGE: AUTOMATED V2X SIMULATOR")
print("="*60)
print(f"[+] USB Link to ESP8266 Transmitter: ACTIVE")
print(f"[+] Wi-Fi Target Acquired: {RPI_IP}:{UDP_PORT}")
print("[+] Initiating Smart City Simulation Sequence...")
time.sleep(2)

phase_timer = time.time()
phase = 1

print("\n[*] PHASE 1: SECURE. Simulating Normal V2X Traffic (Expect GREEN LED)...")

try:
    while True:
        now = time.time()
        
        # 10-Second State Machine to cycle through the 3 LED scenarios
        if (now - phase_timer) > 10:
            phase += 1
            if phase > 3: phase = 1
            phase_timer = now
            
            if phase == 1:
                print("\n[*] PHASE 1: SECURE. Returning to Normal V2X Traffic (Expect GREEN LED)...")
            elif phase == 2:
                print("\n[!] PHASE 2: WARNING. Launching GPS & IoT Spoofing Only (Expect YELLOW LED)...")
            elif phase == 3:
                print("\n[!] PHASE 3: CRITICAL. Launching Full Multi-Vector Compromise (Expect RED LED)...")

        # ==========================================
        # VECTOR 1: CAN BUS INJECTIONS
        # ==========================================
        if phase == 1 or phase == 2:
            speed = hex(random.randint(45, 55))[2:].zfill(2).upper()
            dist = hex(random.randint(18, 22))[2:].zfill(2).upper()
            ser.write(f"164:{speed}00{dist}0000000000\n".encode())
            time.sleep(0.05) 
            
            time_left = hex(random.randint(5, 15))[2:].zfill(2).upper()
            ser.write(f"2B0:0100{time_left}0000000000\n".encode())
            time.sleep(0.05)
            
        elif phase == 3:
            ser.write(b"316:FFFFFF00AABBCCDD\n")
            ser.write(b"164:3200140000000000\n")
            time.sleep(0.01) 

        # ==========================================
        # VECTOR 2 & 3: UDP GPS & IOT INJECTIONS
        # ==========================================
        if phase == 1:
            udp_sock.sendto(b"VECTOR:NORMAL", (RPI_IP, UDP_PORT))
        elif phase == 2 or phase == 3:
            udp_sock.sendto(b"VECTOR:ATTACK", (RPI_IP, UDP_PORT))
            
        time.sleep(0.02)
            
except serial.SerialTimeoutException:
    print("\n[FATAL] Write Timeout! The ESP8266 buffer is frozen.")
except KeyboardInterrupt:
    print("\n[!] Simulation Halted by User.")
finally:
    if ser.is_open:
        ser.close()
    udp_sock.close()
    print("C2 Disconnected.")