# Automotive Edge IDS: Multi-Layer Cyber-Physical Security

Modern connected vehicles are vulnerable across multiple attack surfaces, including internal networks (CAN bus), wireless infotainment systems (IoT), and navigation signals (GNSS). Most existing Intrusion Detection Systems (IDS) monitor these vectors in isolation and require cloud connectivity or heavy computing power, making them impractical for standard automotive electronic control units (ECUs).

This repository contains the code and models for a unified, multi-layer IDS. It evaluates the CAN bus, Wi-Fi networks, and GPS signals simultaneously on a single edge node. By applying strict model compression techniques, the system runs deep learning inference locally at sub-millisecond speeds.

## Core Features

* Multi-Vector Detection: Concurrently monitors the internal CAN bus, external Wi-Fi networks, and satellite GNSS signals for anomalies.
* Model Compression: Uses Response-Level Knowledge Distillation to shrink massive, power-hungry models (LSTMs and DNNs) into highly efficient "Student" models (1D-CNNs and MLPs).
* INT8 Dynamic Quantization: Model weights are quantized to 8-bit integers, dropping the total physical footprint to under 70 KB and reducing the parameter count by over 90%.
* Real-Time Execution: Achieves sub-millisecond inference per model natively on a Raspberry Pi CPU, staying safely within the 20ms automotive real-time constraint.
* Compound Threat Fusion: Merges telemetry from all three pipelines into a single threat score, triggering immediate physical hardware defenses like GPIO LED lockouts and STM32 OLED alerts.

## Hardware-in-the-Loop (HIL) Architecture

The system is built and validated on a physical testbed to simulate a realistic vehicular cyberattack chain.

* Command & Control (C2) Attacker Node: A laptop running Python scripts to generate attack sequences, broadcasting them wirelessly via an ESP8266 module.
* Compromised Gateway: An ESP32 acting as a Multi-ECU simulator. It bridges the wireless attack payloads directly onto the physical copper CAN bus.
* Edge IDS Node: A Raspberry Pi 5 equipped with an MCP2515 CAN transceiver. It runs the INT8 ONNX models, processes traffic, and calculates the compound threat score.
* Victim ECU: An STM32F401RE Nucleo board running bare-metal C code. It generates synthetic vehicle telemetry and uses an I2C OLED display to visualize network lockouts when an attack is detected.

## The Machine Learning Pipeline

To ensure the models learned genuine attack patterns rather than memorizing timelines, all datasets were split chronologically (85% train / 15% validation). Standard scalers were fitted exclusively on the training sets to prevent data leakage.

* Internal Network (CAN Bus): Trained on the OTIDS dataset to detect DoS, Fuzzy, and Impersonation attacks using a 29-frame sliding window. (Teacher: LSTM -> Student: 1D-CNN).
* External IoT Network: Trained on the Edge-IIoTset to classify 14 distinct cyberattacks. IP and MAC identifiers were stripped to prevent hardware overfitting. (Teacher: DNN -> Student: MLP).
* GNSS Navigation: Trained on the UND AV-GPS dataset using 41 signal-level features (such as pseudorange and Doppler shifts) for spoofing detection. (Teacher: DNN -> Student: MLP Ensemble).

## Repository Structure

    Automotive-Edge-IDS/
    ├── C2_Simulator/                 # Python scripts for the attacker laptop
    ├── Edge_Node_RPi/                # Raspberry Pi master script and INT8 ONNX models
    ├── Victim_ECU_STM32/             # Bare-metal C code for the victim STM32 ECU
    ├── ESP32_Gateway/                # Arduino firmware for the compromised CAN gateway
    ├── ESP8266_Attacker/             # Arduino firmware for the Wi-Fi attack broadcaster
    └── README.md                     # Project documentation

## Deployment and Setup

### 1. Edge IDS Setup (Raspberry Pi)
Ensure your Raspberry Pi has its physical CAN interface enabled and set to 125kbps. Open your terminal and run:

    sudo ip link set can0 up type can bitrate 125000

Start the master IDS script:

    sudo python3 master_ids.py

### 2. Victim ECU Setup (STM32)
Compile and flash the bare-metal C code located in `Victim_ECU_STM32/main.c` to your STM32 microcontroller. Verify that your MCP2515 SPI pins and OLED I2C pins match the definitions in the code.

### 3. Gateway & Attacker Setup (ESP32/ESP8266)
Flash the respective Arduino sketches to your ESP32 and ESP8266 modules using the Arduino IDE.

### 4. Launching the Simulation (PC)
From your laptop, execute the automated C2 script to begin the 3-phase threat escalation sequence:

    python attack_automated.py