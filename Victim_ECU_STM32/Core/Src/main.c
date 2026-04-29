#include "stm32f4xx.h"
#include <stdint.h>

// ==============================================================================
// 1. MACROS AND REGISTER DEFINITIONS
// ==============================================================================
#define MCP_RESET       0xC0
#define MCP_WRITE       0x02
#define MCP_READ        0x03
#define CNF1            0x2A
#define CNF2            0x29
#define CNF3            0x28
#define CANCTRL         0x0F
#define CANINTF         0x2C
#define TXB0CTRL        0x30
#define TXB0SIDH        0x31
#define RXB0CTRL        0x60
#define RXB0SIDH        0x61

#define OLED_ADDR       0x78

/* DECLARATION ONLY — defined in stm32f4xx_it.c */
extern volatile uint32_t msTicks;

void Delay_ms(uint32_t ms) {
    uint32_t start = msTicks;
    while ((msTicks - start) < ms);
}

// ==============================================================================
// 2. BARE-METAL I2C & SSD1306 OLED DRIVER
// ==============================================================================

static const uint8_t Font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00,
    0x14, 0x7F, 0x14, 0x7F, 0x14, 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62,
    0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x00,
    0x00, 0x41, 0x22, 0x1C, 0x00, 0x14, 0x08, 0x3E, 0x08, 0x14, 0x08, 0x08, 0x3E, 0x08, 0x08,
    0x00, 0x50, 0x30, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x60, 0x60, 0x00, 0x00,
    0x20, 0x10, 0x08, 0x04, 0x02, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00,
    0x42, 0x61, 0x51, 0x49, 0x46, 0x21, 0x41, 0x45, 0x4B, 0x31, 0x18, 0x14, 0x12, 0x7F, 0x10,
    0x27, 0x45, 0x45, 0x45, 0x39, 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x01, 0x71, 0x09, 0x05, 0x03,
    0x36, 0x49, 0x49, 0x49, 0x36, 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x36, 0x36, 0x00, 0x00,
    0x00, 0x56, 0x36, 0x00, 0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x14, 0x14, 0x14, 0x14, 0x14,
    0x00, 0x41, 0x22, 0x14, 0x08, 0x02, 0x01, 0x51, 0x09, 0x06, 0x32, 0x49, 0x79, 0x41, 0x3E,
    0x7E, 0x11, 0x11, 0x11, 0x7E, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E, 0x41, 0x41, 0x41, 0x22,
    0x7F, 0x41, 0x41, 0x22, 0x1C, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x7F, 0x09, 0x09, 0x09, 0x01,
    0x3E, 0x41, 0x49, 0x49, 0x7A, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F, 0x41, 0x00,
    0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x7F, 0x40, 0x40, 0x40, 0x40,
    0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E, 0x41, 0x41, 0x41, 0x3E,
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x7F, 0x09, 0x19, 0x29, 0x46,
    0x46, 0x49, 0x49, 0x49, 0x31, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x3F, 0x40, 0x40, 0x40, 0x3F,
    0x1F, 0x20, 0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x63, 0x14, 0x08, 0x14, 0x63,
    0x07, 0x08, 0x70, 0x08, 0x07, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x7F, 0x41, 0x41, 0x00,
    0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41, 0x7F, 0x00, 0x04, 0x02, 0x01, 0x02, 0x04,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x01, 0x02, 0x04, 0x00, 0x20, 0x54, 0x54, 0x54, 0x78,
    0x7F, 0x48, 0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x20, 0x38, 0x44, 0x44, 0x48, 0x7F,
    0x38, 0x54, 0x54, 0x54, 0x18, 0x08, 0x7E, 0x09, 0x01, 0x02, 0x0C, 0x52, 0x52, 0x52, 0x3E,
    0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7D, 0x40, 0x00, 0x20, 0x40, 0x44, 0x3D, 0x00,
    0x7F, 0x10, 0x28, 0x44, 0x00, 0x00, 0x41, 0x7F, 0x40, 0x00, 0x7C, 0x04, 0x18, 0x04, 0x78,
    0x7C, 0x08, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38, 0x7C, 0x14, 0x14, 0x14, 0x08,
    0x08, 0x14, 0x14, 0x18, 0x7C, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x48, 0x54, 0x54, 0x54, 0x20,
    0x04, 0x3F, 0x44, 0x40, 0x20, 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x1C, 0x20, 0x40, 0x20, 0x1C,
    0x3C, 0x40, 0x30, 0x40, 0x3C, 0x44, 0x28, 0x10, 0x28, 0x44, 0x0C, 0x50, 0x50, 0x50, 0x3C,
    0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00,
    0x00, 0x41, 0x36, 0x08, 0x00, 0x10, 0x08, 0x08, 0x10, 0x08
};

void I2C1_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB8=SCL, PB9=SDA: AF4, open-drain, pull-up, high speed */
    GPIOB->MODER  &= ~((3U << (8*2)) | (3U << (9*2)));
    GPIOB->MODER  |=  ((2U << (8*2)) | (2U << (9*2)));
    GPIOB->OTYPER |=  ((1U << 8) | (1U << 9));
    GPIOB->OSPEEDR|=  ((3U << (8*2)) | (3U << (9*2)));
    GPIOB->PUPDR  &= ~((3U << (8*2)) | (3U << (9*2)));
    GPIOB->PUPDR  |=  ((1U << (8*2)) | (1U << (9*2)));
    GPIOB->AFR[1] &= ~((0xFU << ((8-8)*4)) | (0xFU << ((9-8)*4)));
    GPIOB->AFR[1] |=  ((4U  << ((8-8)*4)) | (4U  << ((9-8)*4)));

    /* Reset I2C1 peripheral */
    I2C1->CR1 |=  I2C_CR1_SWRST;
    for (volatile int i = 0; i < 200; i++);
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    /* 100 kHz @ PCLK1 16 MHz */
    I2C1->CR2   = 16;
    I2C1->CCR   = 80;
    I2C1->TRISE = 17;
    I2C1->CR1  |= I2C_CR1_PE;
}

void I2C_Write(uint8_t dev_addr, uint8_t control_byte, uint8_t data) {
    while (I2C1->SR2 & I2C_SR2_BUSY);
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = dev_addr;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;
    while (!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = control_byte;
    while (!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = data;
    while (!(I2C1->SR1 & I2C_SR1_BTF));
    I2C1->CR1 |= I2C_CR1_STOP;
    for (volatile int i = 0; i < 50; i++);  /* let STOP complete */
}

void OLED_Command(uint8_t cmd)  { I2C_Write(OLED_ADDR, 0x00, cmd); }
void OLED_Data(uint8_t data)    { I2C_Write(OLED_ADDR, 0x40, data); }

void OLED_Init(void) {
    Delay_ms(120);
    OLED_Command(0xAE);         /* display off */
    OLED_Command(0xD5); OLED_Command(0x80);
    OLED_Command(0xA8); OLED_Command(0x3F);
    OLED_Command(0xD3); OLED_Command(0x00);
    OLED_Command(0x40);
    OLED_Command(0x8D); OLED_Command(0x14); /* charge pump on */
    OLED_Command(0x20); OLED_Command(0x00); /* horizontal addressing */
    OLED_Command(0xA1);                     /* segment remap */
    OLED_Command(0xC8);                     /* COM remap */
    OLED_Command(0xDA); OLED_Command(0x12);
    OLED_Command(0x81); OLED_Command(0xCF);
    OLED_Command(0xD9); OLED_Command(0xF1);
    OLED_Command(0xDB); OLED_Command(0x40);
    OLED_Command(0xA4);                     /* follow RAM */
    OLED_Command(0xA6);                     /* normal (non-inverted) */
    OLED_Command(0xAF);                     /* display on */
    Delay_ms(10);
}

void OLED_SetCursor(uint8_t col, uint8_t page) {
    OLED_Command(0xB0 | (page & 0x07));
    OLED_Command(0x00 |  (col & 0x0F));
    OLED_Command(0x10 | ((col >> 4) & 0x0F));
}

void OLED_Clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        OLED_SetCursor(0, page);
        for (uint8_t col = 0; col < 128; col++)
            OLED_Data(0x00);
    }
}

void OLED_DrawChar(char c) {
    if (c < 32 || c > 126) return;
    uint16_t offset = (uint16_t)(c - 32) * 5;
    for (uint8_t i = 0; i < 5; i++)
        OLED_Data(Font5x7[offset + i]);
    OLED_Data(0x00);
}

void OLED_WriteString(uint8_t col, uint8_t page, const char *str) {
    OLED_SetCursor(col, page);
    while (*str)
        OLED_DrawChar(*str++);
}

// ==============================================================================
// 3. BARE-METAL SPI1 & MCP2515 DRIVER
// ==============================================================================

void SPI1_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* PA5=SCK, PA6=MISO, PA7=MOSI — AF5 */
    GPIOA->MODER &= ~((3U << (5*2)) | (3U << (6*2)) | (3U << (7*2)));
    GPIOA->MODER |=  ((2U << (5*2)) | (2U << (6*2)) | (2U << (7*2)));
    GPIOA->AFR[0] &= ~((0xFU << (5*4)) | (0xFU << (6*4)) | (0xFU << (7*4)));
    GPIOA->AFR[0] |=  ((5U  << (5*4)) | (5U  << (6*4)) | (5U  << (7*4)));

    /* PB6 = CS, output, default high */
    GPIOB->MODER &= ~(3U << (6*2));
    GPIOB->MODER |=  (1U << (6*2));
    GPIOB->ODR   |=  (1U << 6);

    /* SPI1: master, software NSS, fPCLK/8, CPOL=0 CPHA=0 */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1 | SPI_CR1_BR_0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI1_Transfer(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)SPI1->DR;
}

void MCP2515_WriteReg(uint8_t reg, uint8_t val) {
    GPIOB->BSRR = (1U << (6 + 16));  /* CS low */
    SPI1_Transfer(MCP_WRITE);
    SPI1_Transfer(reg);
    SPI1_Transfer(val);
    GPIOB->BSRR = (1U << 6);         /* CS high */
}

uint8_t MCP2515_ReadReg(uint8_t reg) {
    uint8_t val;
    GPIOB->BSRR = (1U << (6 + 16));
    SPI1_Transfer(MCP_READ);
    SPI1_Transfer(reg);
    val = SPI1_Transfer(0xFF);
    GPIOB->BSRR = (1U << 6);
    return val;
}

void MCP2515_Init(void) {
    GPIOB->BSRR = (1U << (6 + 16));
    SPI1_Transfer(MCP_RESET);
    GPIOB->BSRR = (1U << 6);
    Delay_ms(10);

    MCP2515_WriteReg(CANCTRL, 0x80);  /* config mode */
    MCP2515_WriteReg(CNF1, 0x01);
    MCP2515_WriteReg(CNF2, 0xB8);
    MCP2515_WriteReg(CNF3, 0x05);
    MCP2515_WriteReg(RXB0CTRL, 0x60); /* accept all */
    MCP2515_WriteReg(CANCTRL, 0x00);  /* normal mode */
}

void MCP2515_SendCANFrame(uint16_t id, uint8_t *data, uint8_t dlc) {
    MCP2515_WriteReg(TXB0SIDH,     (id >> 3) & 0xFF);
    MCP2515_WriteReg(TXB0SIDH + 1, (id & 0x07) << 5);
    MCP2515_WriteReg(TXB0SIDH + 4, dlc & 0x0F);
    for (int i = 0; i < dlc; i++)
        MCP2515_WriteReg(TXB0SIDH + 5 + i, data[i]);
    MCP2515_WriteReg(TXB0CTRL, 0x08);  /* request to send */
}

uint8_t MCP2515_CheckReceive(void) {
    return (MCP2515_ReadReg(CANINTF) & 0x01) ? 1 : 0;
}

void MCP2515_ReceiveCANFrame(uint16_t *id, uint8_t *data, uint8_t *dlc) {
    uint8_t sidh = MCP2515_ReadReg(RXB0SIDH);
    uint8_t sidl = MCP2515_ReadReg(RXB0SIDH + 1);
    *id  = ((uint16_t)sidh << 3) | (sidl >> 5);
    *dlc = MCP2515_ReadReg(RXB0SIDH + 4) & 0x0F;
    for (int i = 0; i < *dlc; i++)
        data[i] = MCP2515_ReadReg(RXB0SIDH + 5 + i);
    /* clear RX0IF */
    MCP2515_WriteReg(CANINTF, MCP2515_ReadReg(CANINTF) & ~0x01);
}

// ==============================================================================
// 4. BARE-METAL THREAT INDICATOR LOGIC
// ==============================================================================
void LED_Init(void) {
    // Enable GPIOA clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // Set PA0, PA1, PA4 as General Purpose Output (01)
    GPIOA->MODER &= ~((3U << (0*2)) | (3U << (1*2)) | (3U << (4*2)));
    GPIOA->MODER |=  ((1U << (0*2)) | (1U << (1*2)) | (1U << (4*2)));

    // Safely clear LEDs using BSRR (Writing 1 to upper 16 bits resets the pin)
    GPIOA->BSRR = (1U << (0 + 16)) | (1U << (1 + 16)) | (1U << (4 + 16));
}

// Safe toggle function forcing OLED and LEDs to stay perfectly synced
void Set_Threat_State(uint8_t state) {
    // 1. Turn all LEDs off
    GPIOA->BSRR = (1U << (0 + 16)) | (1U << (1 + 16)) | (1U << (4 + 16));

    // 2. Set the requested LED and update the OLED text
    if (state == 0) {
        GPIOA->BSRR = (1U << 0); // Green ON (PA0)
        OLED_WriteString(0, 6, "NET  : SECURE       ");
    } else if (state == 1) {
        GPIOA->BSRR = (1U << 1); // Yellow ON (PA1)
        OLED_WriteString(0, 6, "NET  : ANOMALY!     ");
    } else if (state == 2) {
        GPIOA->BSRR = (1U << 4); // Red ON (PA4)
        OLED_WriteString(0, 6, "NET  : UNDER ATTACK!");
    }
}

// ==============================================================================
// 5. MAIN
// ==============================================================================

int main(void) {
    /* 1 ms SysTick — handler is in stm32f4xx_it.c */
    SysTick_Config(SystemCoreClock / 1000UL);

    SPI1_Init();
    MCP2515_Init();
    LED_Init();

    I2C1_Init();
    OLED_Init();
    OLED_Clear();

    OLED_WriteString(0, 0, "STM32 ECU ONLINE");
    OLED_WriteString(0, 2, "SPEED: ");
    OLED_WriteString(0, 4, "RX ID: 0x---");

    uint8_t  can_payload[8];
    uint32_t last_tx_time = 0;
    uint16_t fake_rpm   = 1000;
    uint8_t  fake_speed = 0;
    int8_t   speed_dir  = 1;

    char val_str[10];
    const char hex_chars[] = "0123456789ABCDEF";

    // Auto-healing variables
    uint8_t is_under_attack = 0;
    uint32_t last_attack_time = 0;

    // Boot to Secure State
    Set_Threat_State(0);

    while (1) {

        /* --- ASYNCHRONOUS CAN POLLING --- */
        if (MCP2515_CheckReceive()) {
            uint16_t rx_id;
            uint8_t  rx_dlc;
            uint8_t  rx_data[8];

            MCP2515_ReceiveCANFrame(&rx_id, rx_data, &rx_dlc);

            // Display most recent ID on OLED
            char rx_str[4];
            rx_str[0] = hex_chars[(rx_id >> 8) & 0x0F];
            rx_str[1] = hex_chars[(rx_id >> 4) & 0x0F];
            rx_str[2] = hex_chars[ rx_id        & 0x0F];
            rx_str[3] = '\0';
            OLED_WriteString(54, 4, rx_str);

            // 1. Physical Hardware Tripwire (Immediate DoS Response)
            if (rx_id == 0x000) {
                Set_Threat_State(2); // Red
                is_under_attack = 1;
                last_attack_time = msTicks;
            }
            // 2. Raspberry Pi AI Broadcast (Calculated Threat Response)
            else if (rx_id == 0x080) {
                Set_Threat_State(rx_data[0]);

                if (rx_data[0] == 0) {
                    is_under_attack = 0; // AI officially cleared the threat
                } else {
                    is_under_attack = 1;
                    last_attack_time = msTicks;
                }
            }
            // Note: We actively IGNORE benign traffic (0x1A4, 0x3C2) here.
            // They will not overwrite the OLED or freeze the LEDs anymore.
        }

        /* --- AUTO-HEAL TIMEOUT --- */
        // If 2 seconds pass without an attack frame or an active AI alert, reset to Secure.
        // This ensures the red LED never gets stuck if the Raspberry Pi crashes.
        if (is_under_attack && ((msTicks - last_attack_time) > 2000)) {
            Set_Threat_State(0);
            is_under_attack = 0;
        }

        /* --- TIMED TRANSMIT AND DISPLAY UPDATE (4 Hz) --- */
        if ((msTicks - last_tx_time) >= 250UL) {
            last_tx_time = msTicks;

            fake_rpm += 50;
            if (fake_rpm > 4000) fake_rpm = 1000;

            fake_speed += (uint8_t)speed_dir;
            if (fake_speed > 120 || fake_speed == 0) speed_dir = (int8_t)(-speed_dir);

            can_payload[0] = (fake_rpm >> 8) & 0xFF;
            can_payload[1] =  fake_rpm       & 0xFF;
            can_payload[2] =  fake_speed;
            can_payload[3] = 0x00;
            can_payload[4] = 0xAA;
            can_payload[5] = 0xBB;
            can_payload[6] = 0xCC;
            can_payload[7] = 0xDD;

            MCP2515_SendCANFrame(0x316, can_payload, 8);

            val_str[0] = (char)((fake_speed / 100)       + '0');
            val_str[1] = (char)(((fake_speed / 10) % 10) + '0');
            val_str[2] = (char)((fake_speed % 10)        + '0');
            val_str[3] = ' '; val_str[4] = 'K';
            val_str[5] = 'M'; val_str[6] = 'H'; val_str[7] = '\0';
            OLED_WriteString(42, 2, val_str);
        }
    }
}
