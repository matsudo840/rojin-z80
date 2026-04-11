#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

/* --- Pin Definitions (From pin_map.md) --- */
const uint8_t ADDR_PINS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
const uint8_t DATA_PINS[] = {17, 18, 21, 38, 39, 40, 41, 42};

#define CLK_PIN    0
#define WAIT_PIN   45
#define RESET_PIN  46
#define MREQ_PIN   47
#define RD_PIN     48
#define WR_PIN     16
#define IORQ_PIN   35
#define DIR_PIN    15

#define OLED_SDA   36
#define OLED_SCL   37
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

uint8_t virtual_memory[0x10000];

const uint8_t z80_program[] = {
    0x3E, 0x4D,       // LD A, 'M'
    0xD3, 0x80,       // OUT (0x80), A
    0x3E, 0x33,       // LD A, '3'
    0xD3, 0x80,       // OUT (0x80), A
    0x3E, 0x21,       // LD A, '!'
    0xD3, 0x80,       // OUT (0x80), A
    0x76              // HALT
};

/* --- Helper Functions --- */

void setDataBusMode(uint8_t mode) {
    for (int i = 0; i < 8; i++) pinMode(DATA_PINS[i], mode);
    digitalWrite(DIR_PIN, (mode == INPUT) ? HIGH : LOW);
}

uint16_t getAddress() {
    uint16_t addr = 0;
    for (int i = 0; i < 14; i++) {
        if (digitalRead(ADDR_PINS[i])) addr |= (1 << i);
    }
    return addr;
}

uint8_t getData() {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (digitalRead(DATA_PINS[i])) data |= (1 << i);
    }
    return data;
}

void setData(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        digitalWrite(DATA_PINS[i], (data >> i) & 0x01);
    }
}

// 待ち合わせ中もクロックを1回振るための関数
void pulseClock() {
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(10);
}

void setup() {
    Serial.begin(115200);
    
    pinMode(CLK_PIN, OUTPUT);
    pinMode(WAIT_PIN, OUTPUT);
    pinMode(RESET_PIN, OUTPUT);
    pinMode(MREQ_PIN, INPUT);
    pinMode(RD_PIN, INPUT);
    pinMode(WR_PIN, INPUT);
    pinMode(IORQ_PIN, INPUT);
    pinMode(DIR_PIN, OUTPUT);

    digitalWrite(WAIT_PIN, HIGH);
    digitalWrite(RESET_PIN, LOW);

    for (int i = 0; i < 14; i++) pinMode(ADDR_PINS[i], INPUT);
    setDataBusMode(INPUT);

    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("OLED failed"));
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(F("Z80 OS v1.1"));
    display.display();

    memset(virtual_memory, 0, sizeof(virtual_memory));
    memcpy(virtual_memory, z80_program, sizeof(z80_program));

    Serial.println("Releasing Reset...");
    for(int i=0; i<100; i++) { pulseClock(); }
    digitalWrite(RESET_PIN, HIGH);
}

void loop() {
    // クロックを振る
    digitalWrite(CLK_PIN, LOW);
    
    // --- Memory Access ---
    if (digitalRead(MREQ_PIN) == LOW) {
        uint16_t addr = getAddress();
        if (digitalRead(RD_PIN) == LOW) {
            uint8_t data = virtual_memory[addr];
            setDataBusMode(OUTPUT);
            setData(data);
            Serial.printf("MEM RD [%04X] -> %02X\n", addr, data);
            // Z80がRDを戻すまでクロックを振りながら待つ
            while(digitalRead(RD_PIN) == LOW) {
                digitalWrite(CLK_PIN, HIGH); delayMicroseconds(5);
                digitalWrite(CLK_PIN, LOW);  delayMicroseconds(5);
            }
            setDataBusMode(INPUT);
        } 
        else if (digitalRead(WR_PIN) == LOW) {
            uint8_t data = getData();
            virtual_memory[addr] = data;
            Serial.printf("MEM WR [%04X] <- %02X\n", addr, data);
            while(digitalRead(WR_PIN) == LOW) {
                digitalWrite(CLK_PIN, HIGH); delayMicroseconds(5);
                digitalWrite(CLK_PIN, LOW);  delayMicroseconds(5);
            }
        }
    }

    // --- I/O Access ---
    if (digitalRead(IORQ_PIN) == LOW) {
        uint16_t port = getAddress() & 0xFF;
        if (digitalRead(WR_PIN) == LOW) {
            uint8_t data = getData();
            Serial.printf("IO OUT [%02X] <- %02X ('%c')\n", port, data, (char)data);
            if (port == 0x80) {
                // OLED描画中はWAITでZ80を止める
                digitalWrite(WAIT_PIN, LOW);
                display.write((char)data);
                display.display();
                digitalWrite(WAIT_PIN, HIGH);
            }
            while(digitalRead(WR_PIN) == LOW) {
                digitalWrite(CLK_PIN, HIGH); delayMicroseconds(5);
                digitalWrite(CLK_PIN, LOW);  delayMicroseconds(5);
            }
        }
    }

    digitalWrite(CLK_PIN, HIGH);
    delay(5); // Debug speed
}
