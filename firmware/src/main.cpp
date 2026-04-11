#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/* --- Pin Definitions (Clean Clock: GPIO 14 = CLK, GPIO 0 = A13) --- */
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

// Try to cover both possible LED pins (38 and 48)
Adafruit_NeoPixel led48(1, 48, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led38(1, 38, NEO_GRB + NEO_KHZ800);

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

void setDataBus(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        digitalWrite(DATA_PINS[i], (data >> i) & 0x01);
    }
}

uint16_t getAddress() {
    uint16_t addr = 0;
    for (int i = 0; i < 14; i++) {
        if (digitalRead(ADDR_PINS[i])) addr |= (1 << i);
    }
    return addr;
}

void pulseClock() {
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(10);
}

void setup() {
    Serial.begin(115200);

    // 1. Immediately hold Z80 in RESET to quiet the bus
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, LOW);
    delay(100);

    // 2. Kill the lights on both candidate pins
    Serial.println("Extinguishing onboard LEDs...");
    led48.begin(); led48.setPixelColor(0, 0); led48.show();
    led38.begin(); led38.setPixelColor(0, 0); led38.show();
    delay(100);
    
    // 3. Configure other pins
    pinMode(CLK_PIN, OUTPUT);
    pinMode(WAIT_PIN, OUTPUT);
    pinMode(MREQ_PIN, INPUT);
    pinMode(RD_PIN, INPUT);
    pinMode(WR_PIN, INPUT);
    pinMode(IORQ_PIN, INPUT);
    pinMode(DIR_PIN, OUTPUT);

    digitalWrite(WAIT_PIN, HIGH);
    
    for (int i = 0; i < 14; i++) pinMode(ADDR_PINS[i], INPUT);
    
    setDataBusMode(OUTPUT);
    setDataBus(0x00); // NOP

    Serial.println("RojinZ80: Free Run Mode (LEDs Off, Clock on G14)");
    
    // RESET sequence with clock pulses
    for(int i = 0; i < 100; i++) { pulseClock(); }
    digitalWrite(RESET_PIN, HIGH);
}

void loop() {
    digitalWrite(CLK_PIN, LOW);
    delay(50); 
    digitalWrite(CLK_PIN, HIGH);
    
    if (digitalRead(MREQ_PIN) == LOW && digitalRead(RD_PIN) == LOW) {
        uint16_t addr = getAddress();
        Serial.printf("ADDR: 0x%04X\n", addr);
    }
    delay(50);
}
