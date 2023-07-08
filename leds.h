#include <FastLED.h>
#define NUM_LEDS 8
CRGBArray<NUM_LEDS> leds;
#define PIN_LED 13  //GPIO1 (TX)
#define BRIGHTNESS 96
#define FRAMES_PER_SECOND 120
uint8_t gHue = 0;  // rotating "base color" used by many of the patterns
int counter;

void setupLeds() {
  FastLED.addLeds<NEOPIXEL, PIN_LED>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
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

void ledsRainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);  // insert a delay to keep the framerate modest
  EVERY_N_MILLISECONDS(20) {
    gHue++;
  }  // slowly cycle the "base color" through the rainbow
}

void ledsCountdown() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  FastLED.clear();
  for (int i = 0; i < counter ; i++) {
    leds[i] = CRGB::White;
  }
  //  leds = CRGB::Black;
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);  // insert a delay to keep the framerate modest
  EVERY_N_MILLISECONDS(500) {
    counter++;
  }  // slowly cycle the "base color" through the rainbow
}

void ledsGreen() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  FastLED.showColor(CRGB::Green);
}

void ledsSinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(0, 255, 192); //red
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);  // insert a delay to keep the framerate modest
  EVERY_N_MILLISECONDS(20) {
    gHue++;
  }  // slowly cycle the "base color" through the rainbow
}