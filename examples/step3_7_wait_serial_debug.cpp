#include <Arduino.h>

// For ESP32-S3 direct GPIO register access
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"

// --- Pin Definitions (Single Source of Truth: pin_map.md) ---
const int ADDR_PINS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
const int DATA_PINS[] = {17, 18, 21, 38, 39, 40, 41, 42};
const int CLK_PIN = 0;
const int DIR_PIN = 19;
const int WAIT_PIN = 45;
const int RESET_PIN = 46;
const int MREQ_PIN = 47;
const int RD_PIN = 48;
const int WR_PIN = 20;

// --- Virtual Memory (64KB) ---
uint8_t virtual_memory[0x10000];

// --- Optimized Bus Helper Functions ---

uint8_t current_bus_mode = 255;

void setDataBusMode(uint8_t mode) {
  if (current_bus_mode == mode)
    return;
  current_bus_mode = mode;

  if (mode == OUTPUT) {
    digitalWrite(DIR_PIN, LOW); // ESP -> Z80
    for (int pin : DATA_PINS)
      pinMode(pin, OUTPUT);
  } else {
    for (int pin : DATA_PINS)
      pinMode(pin, INPUT);
    digitalWrite(DIR_PIN, HIGH); // Z80 -> ESP
  }
}

// Ultra-fast address reading using direct register access (ESP32-S3)
inline uint16_t getAddressFast() {
  // A0-A15 are mapped to GPIO 1 through 16 sequentially.
  // We can read all 16 bits at once by shifting the register.
  return (uint16_t)((GPIO.in >> 1) & 0xFFFF);
}

inline uint8_t readDataBusFast() {
  uint32_t gpio_0_31 = GPIO.in;
  uint32_t gpio_32_63 = GPIO.in1.val;

  uint8_t data = 0;
  if (gpio_0_31 & (1 << 17))
    data |= (1 << 0);
  if (gpio_0_31 & (1 << 18))
    data |= (1 << 1);
  if (gpio_0_31 & (1 << 21))
    data |= (1 << 2);
  if (gpio_32_63 & (1 << (38 - 32)))
    data |= (1 << 3);
  if (gpio_32_63 & (1 << (39 - 32)))
    data |= (1 << 4);
  if (gpio_32_63 & (1 << (40 - 32)))
    data |= (1 << 5);
  if (gpio_32_63 & (1 << (41 - 32)))
    data |= (1 << 6);
  if (gpio_32_63 & (1 << (42 - 32)))
    data |= (1 << 7);
  return data;
}

inline void writeDataBusFast(uint8_t data) {
  uint32_t set_0_31 = 0, set_32_63 = 0;
  uint32_t clr_0_31 = 0, clr_32_63 = 0;

  (data & 0x01) ? set_0_31 |= (1 << 17) : clr_0_31 |= (1 << 17);
  (data & 0x02) ? set_0_31 |= (1 << 18) : clr_0_31 |= (1 << 18);
  (data & 0x04) ? set_0_31 |= (1 << 21) : clr_0_31 |= (1 << 21);
  (data & 0x08) ? set_32_63 |= (1 << (38 - 32)) : clr_32_63 |= (1 << (38 - 32));
  (data & 0x10) ? set_32_63 |= (1 << (39 - 32)) : clr_32_63 |= (1 << (39 - 32));
  (data & 0x20) ? set_32_63 |= (1 << (40 - 32)) : clr_32_63 |= (1 << (40 - 32));
  (data & 0x40) ? set_32_63 |= (1 << (41 - 32)) : clr_32_63 |= (1 << (41 - 32));
  (data & 0x80) ? set_32_63 |= (1 << (42 - 32)) : clr_32_63 |= (1 << (42 - 32));

  GPIO.out_w1ts = set_0_31;
  GPIO.out_w1tc = clr_0_31;
  GPIO.out1_w1ts.val = set_32_63;
  GPIO.out1_w1tc.val = clr_32_63;
}

void setup() {
  Serial.begin(115200);

  // Initialize Memory with a simple program:
  memset(virtual_memory, 0, sizeof(virtual_memory));
  virtual_memory[0x0000] = 0x3E; // LD A, 0x55
  virtual_memory[0x0001] = 0x55;
  virtual_memory[0x0002] = 0x00; // NOP
  virtual_memory[0x0003] = 0xC3; // JP 0x0000
  virtual_memory[0x0004] = 0x00;
  virtual_memory[0x0005] = 0x00;

  // Pin Modes
  pinMode(CLK_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(WAIT_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(MREQ_PIN, INPUT);
  pinMode(RD_PIN, INPUT);
  pinMode(WR_PIN, INPUT);

  for (int pin : ADDR_PINS)
    pinMode(pin, INPUT);
  setDataBusMode(OUTPUT);

  // RESET Sequence
  digitalWrite(WAIT_PIN, HIGH);
  digitalWrite(RESET_PIN, LOW);
  for (int i = 0; i < 40; i++) {
    digitalWrite(CLK_PIN, HIGH);
    delay(1);
    digitalWrite(CLK_PIN, LOW);
    delay(1);
  }
  digitalWrite(RESET_PIN, HIGH);
  Serial.println("Z80 Started (SRAM Emulation Mode - Optimised Polling)");
}

unsigned long last_clk_time = 0;
bool clk_high = false;

inline void updateClockFast() {
  unsigned long now = millis();
  if (now - last_clk_time >= 50) {
    clk_high = !clk_high;
    // GPIO 0 is in the 0-31 range.
    if (clk_high) {
      GPIO.out_w1ts = (1 << 0);
    } else {
      GPIO.out_w1tc = (1 << 0);
    }
    last_clk_time = now;
  }
}

void loop() {
  // 1. Asynchronous Clock Generation (10Hz)
  updateClockFast();

  // 2. Memory Emulation with Software Polling WAIT
  uint32_t gpio_32_63 = GPIO.in1.val;

  // Check if MREQ (GPIO 47) is LOW
  if (!(gpio_32_63 & (1 << (47 - 32)))) {
    
    // MREQ is LOW! Immediately assert WAIT (LOW) to freeze Z80
    GPIO.out1_w1tc.val = (1 << (45 - 32));

    // Now wait (poll) until the Z80 asserts /RD or /WR
    bool is_rd = false;
    bool is_wr = false;
    while (true) {
      uint32_t val_32_63 = GPIO.in1.val;
      uint32_t val_0_31 = GPIO.in;
      is_rd = !(val_32_63 & (1 << (48 - 32)));
      is_wr = !(val_0_31 & (1 << 20));
      if (is_rd || is_wr) break;
      updateClockFast(); // KEEP CLOCK TICKING! Otherwise Z80 never reaches RD/WR
    }

    if (is_rd) {
      // Memory READ
      uint16_t addr = getAddressFast();
      uint8_t data = virtual_memory[addr];
      setDataBusMode(OUTPUT);
      writeDataBusFast(data);
      
      // Serial Debug (at 10Hz this is safe)
      Serial.printf("RD %04X: %02X
", addr, data);

      // Release WAIT so Z80 can sample the data and finish the cycle
      GPIO.out1_w1ts.val = (1 << (45 - 32));

      // Wait for MREQ to go HIGH to end cycle so we don't re-trigger
      while (!(GPIO.in1.val & (1 << (47 - 32)))) {
        updateClockFast();
      }
      setDataBusMode(INPUT);

    } else if (is_wr) {
      // Memory WRITE
      uint16_t addr = getAddressFast();
      setDataBusMode(INPUT);
      uint8_t data = readDataBusFast();
      virtual_memory[addr] = data;

      Serial.printf("WR %04X: %02X
", addr, data);

      // Release WAIT so Z80 can finish the cycle
      GPIO.out1_w1ts.val = (1 << (45 - 32));

      // Wait for MREQ to go HIGH
      while (!(GPIO.in1.val & (1 << (47 - 32)))) {
        updateClockFast();
      }
    }
  }
}
