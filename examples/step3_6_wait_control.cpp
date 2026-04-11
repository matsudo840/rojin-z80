#include <Arduino.h>

// --- Pin Definitions (Single Source of Truth: pin_map.md) ---
const int ADDR_PINS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
const int DATA_PINS[] = {17, 18, 21, 38, 39, 40, 41, 42};
const int CLK_PIN = 0;
const int DIR_PIN =
    19; // Level Shifter Direction (Low: ESP->Z80, High: Z80->ESP)
const int WAIT_PIN = 45;
const int RESET_PIN = 46;
const int MREQ_PIN = 47;
const int RD_PIN = 48;
const int WR_PIN = 20;

// --- Virtual Memory (64KB) ---
uint8_t virtual_memory[0x10000];

// --- Bus Helper Functions ---

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

uint16_t getAddress() {
  uint16_t addr = 0;
  for (int i = 0; i < 16; i++) {
    if (digitalRead(ADDR_PINS[i]))
      addr |= (1 << i);
  }
  return addr;
}

uint8_t readDataBus() {
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(DATA_PINS[i]))
      data |= (1 << i);
  }
  return data;
}

void writeDataBus(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(DATA_PINS[i], (data >> i) & 0x01);
  }
}

// --- Interrupt Service Routine ---
void on_mreq_falling() {
  // Immediately halt Z80 to give ESP32 time to emulate SRAM
  digitalWrite(WAIT_PIN, LOW);
}

void setup() {
  Serial.begin(115200);

  // Initialize Memory with a simple program:
  // 0000: 0x3E 0x55  (LD A, 0x55)
  // 0002: 0x00       (NOP)
  // 0003: 0xC3 0x00 0x00 (JP 0x0000)
  memset(virtual_memory, 0, sizeof(virtual_memory));
  virtual_memory[0x0000] = 0x3E;
  virtual_memory[0x0001] = 0x55;
  virtual_memory[0x0002] = 0x00;
  virtual_memory[0x0003] = 0xC3;
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

  // Attach interrupt to /MREQ to halt Z80 on memory requests
  attachInterrupt(digitalPinToInterrupt(MREQ_PIN), on_mreq_falling, FALLING);

  // RESET Sequence (Z80 requires clock cycles while RESET is active)
  digitalWrite(WAIT_PIN, HIGH);
  digitalWrite(RESET_PIN, LOW);

  // Provide clock cycles during reset (approx 2 seconds = 40 cycles of 50ms)
  for (int i = 0; i < 40; i++) {
    digitalWrite(CLK_PIN, HIGH);
    delay(25);
    digitalWrite(CLK_PIN, LOW);
    delay(25);
  }

  digitalWrite(RESET_PIN, HIGH);
  Serial.println("Z80 Started (SRAM Emulation Mode)");
}

void loop() {
  // 1. Asynchronous Clock Generation (10Hz: 50ms HIGH, 50ms LOW)
  static unsigned long last_clk_time = 0;
  static bool clk_high = false;
  unsigned long now = millis();

  if (now - last_clk_time >= 50) {
    clk_high = !clk_high;
    digitalWrite(CLK_PIN, clk_high ? HIGH : LOW);
    last_clk_time = now;
  }

  // 2. Memory Emulation with WAIT Control
  if (digitalRead(MREQ_PIN) == LOW) {
    // Z80 is waiting for us (or is about to be, thanks to the ISR).
    bool rd = (digitalRead(RD_PIN) == LOW);
    bool wr = (digitalRead(WR_PIN) == LOW);

    if (rd) {
      // Memory READ: Z80 wants data, ESP outputs it
      uint16_t addr = getAddress();
      setDataBusMode(OUTPUT);
      writeDataBus(virtual_memory[addr]);
    } else if (wr) {
      // Memory WRITE: Z80 sends data, ESP reads it
      uint16_t addr = getAddress();
      setDataBusMode(INPUT);
      virtual_memory[addr] = readDataBus();
    }

    // Data is ready on the bus (or has been read). Release the WAIT pin.
    // The Z80 will sample the bus on the subsequent clock edges.
    digitalWrite(WAIT_PIN, HIGH);
  } else {
    // /MREQ is HIGH (Memory cycle over). Safe to release the bus.
    // Also ensure WAIT is HIGH to prevent deadlocks when idle.
    setDataBusMode(INPUT);
    digitalWrite(WAIT_PIN, HIGH);
  }
}
