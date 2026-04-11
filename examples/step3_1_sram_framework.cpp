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

void setDataBusMode(uint8_t mode) {
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

  // RESET Sequence
  digitalWrite(WAIT_PIN, HIGH);
  digitalWrite(CLK_PIN, LOW);
  digitalWrite(RESET_PIN, LOW);
  delay(2000);
  digitalWrite(RESET_PIN, HIGH);
  Serial.println("Z80 Started (SRAM Emulation Mode)");
}

void loop() {
  // 1. Clock HIGH (Start of Cycle)
  digitalWrite(CLK_PIN, HIGH);

  // Z80 evaluates logic here...

  // 2. Check for Memory Request
  if (digitalRead(MREQ_PIN) == LOW) {
    uint16_t addr = getAddress();

    if (digitalRead(RD_PIN) == LOW) {
      // Memory READ
      setDataBusMode(OUTPUT);
      writeDataBus(virtual_memory[addr]);
    } else if (digitalRead(WR_PIN) == LOW) {
      // Memory WRITE
      setDataBusMode(INPUT);
      virtual_memory[addr] = readDataBus();
    }
  }

  delay(50); // 10Hz Clock part 1

  // 3. Clock LOW (End of Cycle)
  digitalWrite(CLK_PIN, LOW);

  // Clean up data bus if it was a read cycle to avoid contention later
  setDataBusMode(INPUT);

  delay(50); // 10Hz Clock part 2
}
