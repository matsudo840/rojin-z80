#include <Arduino.h>

// Step 2: Z80 Free Run (Active NOP)
// ESP32のGPIOを使ってデータバスを0x00(NOP)に固定し、Z80のアドレスバスが進むかを確認する。

// --- Pin Definitions (From pin_map.md) ---
const int CLK_PIN = 0;
const int RESET_PIN = 46;
const int WAIT_PIN = 45;
const int DIR_PIN = 19; // Data Bus Direction (LOW = B->A: ESP->Z80)

// Data Bus GPIOs
const int DATA_PINS[] = {17, 18, 21, 38, 39, 40, 41, 42};

void setup() {
  Serial.begin(115200);

  // Configure Control Pins
  pinMode(CLK_PIN, OUTPUT);
  digitalWrite(CLK_PIN, LOW);

  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW); // Hold in RESET initially

  pinMode(WAIT_PIN, OUTPUT);
  digitalWrite(WAIT_PIN, HIGH); // Disable WAIT

  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW); // ESP32 -> Z80 direction (B -> A)

  // Configure Data Bus as Output LOW (NOP: 0x00)
  for (int pin : DATA_PINS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  // Release RESET with a clear delay for the logic analyzer
  Serial.println("Holding Z80 in RESET for 2 seconds...");
  digitalWrite(RESET_PIN, LOW);
  delay(2000);

  Serial.println("Releasing RESET...");
  digitalWrite(RESET_PIN, HIGH);
}

void loop() {
  // Generate slow clock (10Hz)
  digitalWrite(CLK_PIN, HIGH);
  delay(50);
  digitalWrite(CLK_PIN, LOW);
  delay(50);
}
