#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

/* 
 * Roujin Z80: Final Production Edition
 * - Identical timing to the "Perfectly Working" version.
 * - Standalone stable: No serial hangs, no LED interference.
 */

const uint8_t ADDR_PINS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0}; 
const uint8_t DATA_PINS[] = {17, 18, 21, 38, 39, 40, 41, 42};

#define CLK_PIN    14
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

Adafruit_NeoPixel led48(1, 48, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led38(1, 38, NEO_GRB + NEO_KHZ800);

uint8_t virtual_memory[0x4000]; // 16KB

const uint8_t z80_program[] = {
    0x3E, 0x4D,       // 0000: LD A, 'M'
    0xD3, 0x80,       // 0002: OUT (0x80), A
    0x3E, 0x33,       // 0004: LD A, '3'
    0xD3, 0x80,       // 0006: OUT (0x80), A
    0x3E, 0x21,       // 0008: LD A, '!'
    0xD3, 0x80,       // 000A: OUT (0x80), A
    0x3E, 0x20,       // 000C: LD A, ' '
    0xD3, 0x80,       // 000E: OUT (0x80), A
    0xC3, 0x00, 0x00  // 0010: JP 0000
};

inline uint16_t getAddress() {
    uint32_t gpio_in = GPIO.in;
    return ((gpio_in >> 1) & 0x1FFF) | ((gpio_in & 0x01) << 13);
}

void setDataBusMode(uint8_t mode) {
    if (mode == OUTPUT) {
        digitalWrite(DIR_PIN, LOW); 
        delayMicroseconds(1);
        for (int i = 0; i < 8; i++) pinMode(DATA_PINS[i], OUTPUT);
    } else {
        for (int i = 0; i < 8; i++) pinMode(DATA_PINS[i], INPUT);
        delayMicroseconds(1);
        digitalWrite(DIR_PIN, HIGH);
    }
}

inline uint8_t getData() {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (digitalRead(DATA_PINS[i])) data |= (1 << i);
    }
    return data;
}

inline void setData(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        digitalWrite(DATA_PINS[i], (data >> i) & 0x01);
    }
}

void pulseClock() {
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(10);
}

void setup() {
    // Keep Serial active but don't print to avoid buffer issues
    Serial.begin(115200);

    // Disable LEDs
    led48.begin(); led48.setPixelColor(0, 0); led48.show();
    led38.begin(); led38.setPixelColor(0, 0); led38.show();
    
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, LOW);
    
    pinMode(CLK_PIN, OUTPUT);
    pinMode(WAIT_PIN, OUTPUT);
    pinMode(MREQ_PIN, INPUT);
    pinMode(RD_PIN, INPUT);
    pinMode(WR_PIN, INPUT);
    pinMode(IORQ_PIN, INPUT);
    pinMode(DIR_PIN, OUTPUT);

    digitalWrite(WAIT_PIN, HIGH);
    for (int i = 0; i < 14; i++) pinMode(ADDR_PINS[i], INPUT);
    setDataBusMode(INPUT);

    Wire.begin(OLED_SDA, OLED_SCL);
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0,0);
    display.println(F("Roujin Z80"));
    display.setCursor(0, 16);
    display.display();

    memset(virtual_memory, 0, sizeof(virtual_memory));
    memcpy(virtual_memory, z80_program, sizeof(z80_program));

    // Wait for power to stabilize
    delay(500);

    for(int i = 0; i < 100; i++) { pulseClock(); }
    digitalWrite(RESET_PIN, HIGH);
}

void loop() {
    // Clock Fall
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(5);

    // --- I/O Access ---
    if (digitalRead(IORQ_PIN) == LOW) {
        uint16_t port = getAddress() & 0xFF;
        if (digitalRead(WR_PIN) == LOW) {
            uint8_t data = getData();
            if (port == 0x80) {
                digitalWrite(WAIT_PIN, LOW); // Hold Z80
                
                display.write((char)data);
                display.display();
                
                if (display.getCursorY() >= 64) {
                    display.fillRect(0, 16, 128, 48, SSD1306_BLACK);
                    display.setCursor(0, 16);
                    display.display();
                }

                // Mimic the working version's delay rhythm
                for(int i = 0; i < 50; i++) {
                    digitalWrite(CLK_PIN, HIGH); delay(10);
                    digitalWrite(CLK_PIN, LOW);  delay(10);
                }

                digitalWrite(WAIT_PIN, HIGH); // Release Z80
            }
            // Ensure WR is released
            int timeout = 0;
            while(digitalRead(WR_PIN) == LOW && timeout < 1000) { pulseClock(); timeout++; }
        }
    }
    // --- Memory Access ---
    else if (digitalRead(MREQ_PIN) == LOW) {
        uint16_t addr = getAddress();
        if (digitalRead(RD_PIN) == LOW) {
            uint8_t data = virtual_memory[addr & 0x3FFF];
            setDataBusMode(OUTPUT);
            setData(data);
            while(digitalRead(RD_PIN) == LOW) { pulseClock(); }
            setDataBusMode(INPUT);
        } else if (digitalRead(WR_PIN) == LOW) {
            uint8_t data = getData();
            virtual_memory[addr & 0x3FFF] = data;
            while(digitalRead(WR_PIN) == LOW) { pulseClock(); }
        }
    }

    // Clock Rise
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(10);
}
