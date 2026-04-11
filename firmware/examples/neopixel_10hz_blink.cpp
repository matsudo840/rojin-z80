#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

// NeoPixel 10Hz Blink Test (動作確認済み)
// ESP32-S3-DevKitC-1 のオンボード RGB LED (GPIO 48) を
// 10Hzで緑色に点滅させるサンプル。

#define DIN_PIN 48
#define LED_COUNT 1
#define BRIGHTNESS 8

Adafruit_NeoPixel pixels(LED_COUNT, DIN_PIN, NEO_GRB + NEO_KHZ800);

void setup() { pixels.begin(); }

void loop() {
  // 10Hz = 100ms周期 (50ms ON / 50ms OFF)
  pixels.setPixelColor(0, pixels.Color(0, BRIGHTNESS, 0)); // 緑で点灯
  pixels.show();
  delay(50);

  pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // 消灯
  pixels.show();
  delay(50);
}
