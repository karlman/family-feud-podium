#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// ── Hardware constants ────────────────────────────────────────────────────────
#define STRIP_NUM_LEDS  72
#define STRIP_COLS       6
#define STRIP_ROWS      12
#define BAUD_RATE   115200

// ── Pin definitions ───────────────────────────────────────────────────────────
// Podium 1
#define P1_TOP    3   // PWM – top flood light MOSFET
#define P1_FRONT  5   // PWM – front flood light MOSFET
#define P1_BTN    6   // PWM – button backlight MOSFET
#define P1_STRIP  4   // NeoPixel data
#define P1_SW     7   // Button switch (INPUT_PULLUP, active-LOW)

// Podium 2
#define P2_TOP    9   // PWM
#define P2_FRONT 10   // PWM
#define P2_BTN   11   // PWM
#define P2_STRIP  2   // NeoPixel data
#define P2_SW     8   // Button switch

// ── Colour palette ────────────────────────────────────────────────────────────
#define CLR_OFF         (uint32_t)0x000000
#define CLR_ROYAL_BLUE  (uint32_t)0x0A14B4   // r=10 g=20 b=180
#define CLR_WHITE       (uint32_t)0xFFFFFF
#define CLR_RED         (uint32_t)0xFF0000
#define CLR_DIM_BLUE    (uint32_t)0x050A5A   // darker blue for inactive podium

// ── Game states ───────────────────────────────────────────────────────────────
enum GameState : uint8_t {
  GS_CLEAR,     // all off
  GS_RESET,     // both royal-blue, no buzz-in
  GS_BUZZIN,    // pulsing blue, listening for buttons
  GS_ACTIVE_1,  // podium 1 white, podium 2 dim blue
  GS_ACTIVE_2,  // podium 2 white, podium 1 dim blue
  GS_WIN_1,     // rainbow on podium 1
  GS_WIN_2,     // rainbow on podium 2
};

// ── Serpentine pixel index ────────────────────────────────────────────────────
// Even rows: left-to-right.  Odd rows: right-to-left.
inline int serpIdx(int col, int row) {
  return (row % 2 == 0)
    ? row * STRIP_COLS + col
    : row * STRIP_COLS + (STRIP_COLS - 1 - col);
}

// ── Function declarations ─────────────────────────────────────────────────────
void processCommand(const char* cmd);
void setMosfets(int podium, uint8_t val);
void setAllMosfets(uint8_t val);
void fillStrip(Adafruit_NeoPixel& s, uint32_t color);
void flashStrikeX(Adafruit_NeoPixel& s);
void drawPersistentStrikes(Adafruit_NeoPixel& s, int strikes);
void doStartupSequence();
