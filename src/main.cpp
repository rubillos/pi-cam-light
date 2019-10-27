#include <Arduino.h>

#include "Adafruit_NeoPixel.h"
#include "elapsedMillis.h"

#define LED_STRIP_LENGTH 12
#define LED_STRIP_PIN 1
#define ENABLE_PIN 4

#define MAX_BRIGHTNESS 127
#define WHITE_STRIP
#define USE_GAMMA_TABLE

#define FADE_TIME_SECONDS 5
#define ADJUST_DELAY (FADE_TIME_SECONDS * 1000 / 255)

#ifdef WHITE_STRIP
#define STRIP_TYPE NEO_RGBW
#else
#define STRIP_TYPE NEO_GRB
#endif

Adafruit_NeoPixel strip(LED_STRIP_LENGTH, LED_STRIP_PIN, STRIP_TYPE);

uint8_t currentBrightness = 0;
uint8_t desiredBrightness = 0;
uint16_t correctedBrightness = 0;

/* 16-bit gamma-correction table.
   Copy & paste this snippet into a Python REPL to regenerate
   Set the maximum brightness in max_bright:
import math
gamma=2.6
max_bright=127.0
for x in range(256):
    print("{:3},".format(int(math.pow((x)/255.0,gamma)*(max_bright*256.0)+0.5))),
    if x&15 == 15: print
*/

#ifdef USE_GAMMA_TABLE
const PROGMEM uint16_t gammaTable[] = {
  0,   0,   0,   0,   1,   1,   2,   3,   4,   5,   7,   9,  12,  14,  17,  21,
 24,  28,  33,  38,  43,  49,  56,  62,  70,  78,  86,  95, 104, 114, 125, 136,
147, 160, 173, 186, 200, 215, 230, 246, 263, 281, 299, 318, 337, 358, 379, 400,
423, 446, 470, 495, 521, 547, 574, 603, 631, 661, 692, 723, 755, 789, 823, 858,
894, 930, 968, 1007, 1046, 1087, 1128, 1170, 1214, 1258, 1303, 1350, 1397, 1445, 1494, 1545,
1596, 1649, 1702, 1756, 1812, 1869, 1926, 1985, 2045, 2106, 2168, 2231, 2296, 2361, 2428, 2495,
2564, 2634, 2705, 2778, 2851, 2926, 3002, 3079, 3157, 3237, 3318, 3400, 3483, 3567, 3653, 3740,
3828, 3918, 4009, 4101, 4194, 4289, 4385, 4482, 4580, 4680, 4782, 4884, 4988, 5093, 5200, 5308,
5417, 5528, 5640, 5754, 5869, 5985, 6103, 6222, 6342, 6464, 6588, 6712, 6839, 6966, 7096, 7226,
7358, 7492, 7627, 7764, 7902, 8041, 8182, 8325, 8469, 8615, 8762, 8910, 9061, 9213, 9366, 9521,
9677, 9835, 9995, 10156, 10319, 10483, 10649, 10817, 10986, 11157, 11329, 11503, 11679, 11857, 12036, 12216,
12399, 12583, 12768, 12956, 13145, 13335, 13528, 13722, 13918, 14115, 14314, 14515, 14718, 14922, 15129, 15336,
15546, 15758, 15971, 16186, 16402, 16621, 16841, 17063, 17287, 17513, 17740, 17969, 18200, 18433, 18668, 18904,
19143, 19383, 19625, 19869, 20115, 20362, 20612, 20863, 21116, 21371, 21628, 21887, 22148, 22411, 22676, 22942,
23210, 23481, 23753, 24027, 24304, 24582, 24862, 25144, 25428, 25714, 26002, 26292, 26583, 26877, 27173, 27471,
27771, 28073, 28377, 28682, 28990, 29300, 29612, 29926, 30242, 30560, 30880, 31203, 31527, 31853, 32182, 32512
};
#endif

void computeGamma() {
  #ifdef USE_GAMMA_TABLE
    correctedBrightness = pgm_read_word_near(gammaTable + currentBrightness);
    // return gammaTable[currentBrightness];
  #else
    float gammaCorrected = (MAX_BRIGHTNESS * 256.0) * pow((float)currentBrightness / 255.0, 2.6) + 0.5;

    if (gammaCorrected > 65535.0) {
      gammaCorrected = 65535.0;
    }

    correctedBrightness = gammaCorrected;
  #endif
}

#define DITHER_COUNT 16

uint32_t currentColor() {
  uint32_t c;
  uint8_t b = correctedBrightness >> 8;
  uint8_t dither = ((correctedBrightness & 0xFF) * DITHER_COUNT) / 256;
  static uint8_t ditherIndex = 0;

  if ((dither > ditherIndex) && (b < 255)) {
    b++;
  }

  ditherIndex = (ditherIndex + 1) % DITHER_COUNT;

  #ifdef WHITE_STRIP
    c = (uint32_t)b << 24;
  #else
    c = b;
    c <<= 8;
    c |= b;
    c <<= 8;
    c |= b;
  #endif

  return c;
}

void updateStrips() {
  strip.fill(currentColor());
  strip.show();
}

void updateInternalLED() {
  analogWrite(LED_BUILTIN, correctedBrightness>>8);
}

void setup() {
  // Switch from 1Mhz to 8Mhz
  CLKPR = 0x80; // to enable changes
  CLKPR = 0x00; // to set the divisor to 1

  pinMode(ENABLE_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  strip.begin();
  updateStrips();
  updateInternalLED();
}

void loop() {
  static elapsedMillis adjustTime;

  if (adjustTime > ADJUST_DELAY)  {
    adjustTime -= ADJUST_DELAY;

    desiredBrightness = (digitalRead(ENABLE_PIN)==LOW) ? 255 : 0;

    if (desiredBrightness != currentBrightness) {
      if (desiredBrightness > currentBrightness) {
        currentBrightness += 1;
      }
      else {
        currentBrightness -= 1;
      }

      computeGamma();
      updateInternalLED();
    }
  }

  static elapsedMicros updateTime;

  if (updateTime > 1000) {
    updateTime -= 1000;
    updateStrips();
  }
}
