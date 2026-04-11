#include <Arduino.h>

// Step 1: Clock Generation (The Heartbeat)
// GPIO 0 から 10Hz の矩形波を出力する。
// NeoPixel 10Hz点滅で動作確認済みのロジックを、GPIO出力に置き換えたもの。
//
// Note: GPIO 0 は Strapping Pin (BOOT) だが、起動後は通常のGPIOとして使用可能。

const int CLK_PIN = 0;

void setup() { pinMode(CLK_PIN, OUTPUT); }

void loop() {
  // 10Hz = 100ms周期 (50ms HIGH / 50ms LOW)
  digitalWrite(CLK_PIN, HIGH);
  delay(50);
  digitalWrite(CLK_PIN, LOW);
  delay(50);
}
