#include <Arduino.h>

#include <Adafruit_NeoPixel.h>
#include "elapsedMillis.h"

#define LED_STRIP_LENGTH 12
#define LED_STRIP_PIN 1
#define SWITCH_PIN 4

Adafruit_NeoPixel strip(LED_STRIP_LENGTH, LED_STRIP_PIN, NEO_GRB);

#define UPDATES_PER_SECOND 60
#define ADJUST_TIME (750 / UPDATES_PER_SECOND)
#define MAX_BRIGHTNESS 255
#define BRIGHTNESS_SHIFT 0
#define BRIGHTNESS_TAIL 0

uint8_t currentBrightness = 0;
uint16_t correctedBrightness = 0;
uint8_t desiredBrightness = 0;

uint16_t adjustBrightness(uint8_t bright) {
  float gammaCorrected = 65535.0 * pow((float)bright / (float)MAX_BRIGHTNESS, 2.6);

  if (gammaCorrected > 65535.0) {
    gammaCorrected = 65535.0;
  }

  return gammaCorrected;
}

#define DITHER_VALUE 16

uint32_t currentColor() {
  uint32_t c;
  uint8_t b = correctedBrightness >> 8;
  uint8_t dither = (correctedBrightness & 0xFF) * DITHER_VALUE / 255;
  static uint8_t ditherIndex = 0;

  if ((dither > ditherIndex) && (b < 255)) {
    b++;
  }

  ditherIndex = (ditherIndex + 1) % DITHER_VALUE;

  c = b;
  c <<= 8;
  c |= b;
  c <<= 8;
  c |= b;

  return c;
}

void setup() {
  // Switch from 1Mhz to 8Mhz
  CLKPR = 0x80; // to enable changes
  CLKPR = 0x00; // to set the divisor to 1

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  strip.begin();
  strip.fill(currentColor());
  strip.show();

  analogWrite(LED_BUILTIN, correctedBrightness>>8);
}

void loop() {
  static elapsedMillis adjustTime;

  if (adjustTime > ADJUST_TIME)  {
    bool enabled = (digitalRead(SWITCH_PIN)==LOW);

    adjustTime = 0;

    desiredBrightness = (enabled) ? MAX_BRIGHTNESS : 0;

    if (currentBrightness != desiredBrightness) {
      if (desiredBrightness > currentBrightness) {
        currentBrightness += 1;
      }
      else {
        currentBrightness -= 1;
      }

      correctedBrightness = adjustBrightness(currentBrightness);
      analogWrite(LED_BUILTIN, correctedBrightness>>8);
    }
  }

  static elapsedMicros updateTime;

  if (updateTime > 1000) {
    updateTime -= 1000;
    strip.fill(currentColor());
    strip.show();
  }
}
