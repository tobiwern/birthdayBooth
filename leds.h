#include <FastLED.h>
#define NUM_LEDS 8
CRGBArray<NUM_LEDS> leds;
#define PIN_LED 13            //GPIO1 (TX)
#define BRIGHTNESS 96
#define FRAMES_PER_SECOND 120
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void setupLeds() {
  FastLED.addLeds<NEOPIXEL, PIN_LED>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
}

void handleLeds() {
  FastLED.show();
  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  // do some periodic updates
  EVERY_N_MILLISECONDS(20) {gHue++;}  // slowly cycle the "base color" through the rainbow
}

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}