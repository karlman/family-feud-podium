#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <podium.h>

// ── NeoPixel strips ───────────────────────────────────────────────────────────
Adafruit_NeoPixel strip1(STRIP_NUM_LEDS, P1_STRIP, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel strip2(STRIP_NUM_LEDS, P2_STRIP, NEO_RGB + NEO_KHZ800);

// ── State ─────────────────────────────────────────────────────────────────────
static GameState gameState     = GS_CLEAR;
static bool      buzzinEnabled = false;

// ── Serial command buffer ─────────────────────────────────────────────────────
static char    cmdBuf[64];
static uint8_t cmdLen = 0;

// ── Animation timing ──────────────────────────────────────────────────────────
static unsigned long animTimer       = 0;
static int           animPhase       = 0;
static int           pulseBrightness = 30;
static int           pulseDelta      = 5;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(BAUD_RATE);

  const uint8_t mosfetPins[] = { P1_TOP, P1_FRONT, P1_BTN,
                                  P2_TOP, P2_FRONT, P2_BTN };
  for (uint8_t p : mosfetPins) { pinMode(p, OUTPUT); analogWrite(p, 0); }

  pinMode(P1_SW, INPUT_PULLUP);
  pinMode(P2_SW, INPUT_PULLUP);

  strip1.begin(); strip1.show();
  strip2.begin(); strip2.show();

  doStartupSequence();
  Serial.println("READY");
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  // ── Serial command reader ─────────────────────────────────────────────────
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmdLen > 0) {
        cmdBuf[cmdLen] = '\0';
        processCommand(cmdBuf);
        cmdLen = 0;
      }
    } else if (cmdLen < 63) {
      cmdBuf[cmdLen++] = c;
    }
  }

  // ── Buzz-in button polling ────────────────────────────────────────────────
  if (digitalRead(P1_SW) == LOW) {
    if (buzzinEnabled) {
      buzzinEnabled = false;
      Serial.println("RINGER:1");
    } else {
      Serial.println("DEBUG:BTN1_NO_BUZZIN");
    }
    delay(200);
  } else if (digitalRead(P2_SW) == LOW) {
    if (buzzinEnabled) {
      buzzinEnabled = false;
      Serial.println("RINGER:2");
    } else {
      Serial.println("DEBUG:BTN2_NO_BUZZIN");
    }
    delay(200);
  }

  // ── Animations ───────────────────────────────────────────────────────────
  unsigned long now = millis();

  // Pulsing blue during buzz-in
  if (gameState == GS_BUZZIN && now - animTimer > 18) {
    animTimer = now;
    pulseBrightness += pulseDelta;
    if (pulseBrightness >= 210) { pulseBrightness = 210; pulseDelta = -5; }
    else if (pulseBrightness <= 25) { pulseBrightness = 25;  pulseDelta =  5; }

    uint8_t r = (uint8_t)((10  * pulseBrightness) / 210);
    uint8_t g = (uint8_t)((20  * pulseBrightness) / 210);
    uint8_t b = (uint8_t)((180 * pulseBrightness) / 210);
    uint32_t c = strip1.Color(r, g, b);
    fillStrip(strip1, c);
    fillStrip(strip2, c);
  }

  // Rainbow winner animation
  if ((gameState == GS_WIN_1 || gameState == GS_WIN_2) && now - animTimer > 28) {
    animTimer = now;
    animPhase = (animPhase + 1) & 0xFF;

    Adafruit_NeoPixel& winStrip  = (gameState == GS_WIN_1) ? strip1 : strip2;
    Adafruit_NeoPixel& loseStrip = (gameState == GS_WIN_1) ? strip2 : strip1;

    for (int i = 0; i < STRIP_NUM_LEDS; i++) {
      uint16_t hue = (uint16_t)(animPhase * 256 + i * 65536 / STRIP_NUM_LEDS);
      winStrip.setPixelColor(i, winStrip.gamma32(winStrip.ColorHSV(hue)));
    }
    winStrip.show();
    fillStrip(loseStrip, CLR_OFF);

    int winPodium = (gameState == GS_WIN_1) ? 1 : 2;
    uint8_t pulse = (animPhase & 0x3F) < 32 ? 255 : 140;
    setMosfets(winPodium,              pulse);
    setMosfets(winPodium == 1 ? 2 : 1, 0);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
void processCommand(const char* cmd) {
  if (strcmp(cmd, "RESET") == 0) {
    gameState     = GS_RESET;
    buzzinEnabled = false;
    fillStrip(strip1, CLR_ROYAL_BLUE);
    fillStrip(strip2, CLR_ROYAL_BLUE);
    setAllMosfets(0);

  } else if (strcmp(cmd, "BUZZIN") == 0) {
    gameState       = GS_BUZZIN;
    buzzinEnabled   = true;
    pulseBrightness = 30;
    pulseDelta      = 5;
    animTimer       = millis();

  } else if (strcmp(cmd, "ACTIVE:1") == 0) {
    gameState     = GS_ACTIVE_1;
    buzzinEnabled = false;
    fillStrip(strip1, CLR_WHITE);
    fillStrip(strip2, CLR_DIM_BLUE);
    setMosfets(1, 255);
    setMosfets(2, 60);

  } else if (strcmp(cmd, "ACTIVE:2") == 0) {
    gameState     = GS_ACTIVE_2;
    buzzinEnabled = false;
    fillStrip(strip2, CLR_WHITE);
    fillStrip(strip1, CLR_DIM_BLUE);
    setMosfets(2, 255);
    setMosfets(1, 60);

  } else if (strncmp(cmd, "STRIKE:", 7) == 0) {
    // Flash red X on both strips; Pi will follow with ACTIVE: to restore state
    flashStrikeX(strip1);
    flashStrikeX(strip2);
    delay(700);
    fillStrip(strip1, CLR_ROYAL_BLUE);
    fillStrip(strip2, CLR_ROYAL_BLUE);
    setAllMosfets(0);
    gameState = GS_RESET;

  } else if (strcmp(cmd, "WIN:1") == 0) {
    gameState = GS_WIN_1;
    animPhase = 0;
    animTimer = millis();

  } else if (strcmp(cmd, "WIN:2") == 0) {
    gameState = GS_WIN_2;
    animPhase = 0;
    animTimer = millis();

  } else if (strcmp(cmd, "CLEAR") == 0) {
    gameState     = GS_CLEAR;
    buzzinEnabled = false;
    fillStrip(strip1, CLR_OFF);
    fillStrip(strip2, CLR_OFF);
    setAllMosfets(0);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
void setMosfets(int podium, uint8_t val) {
  if (podium == 1) {
    analogWrite(P1_TOP,   val);
    analogWrite(P1_FRONT, val);
    analogWrite(P1_BTN,   val);
  } else {
    analogWrite(P2_TOP,   val);
    analogWrite(P2_FRONT, val);
    analogWrite(P2_BTN,   val);
  }
}

void setAllMosfets(uint8_t val) {
  setMosfets(1, val);
  setMosfets(2, val);
}

void fillStrip(Adafruit_NeoPixel& s, uint32_t color) {
  s.fill(color);
  s.show();
}

// ── X pattern drawn across the 6×12 serpentine grid ──────────────────────────
void flashStrikeX(Adafruit_NeoPixel& s) {
  s.clear();
  for (int row = 0; row < STRIP_ROWS; row++) {
    int colA = (row * (STRIP_COLS - 1)) / (STRIP_ROWS - 1);
    int colB = (STRIP_COLS - 1) - colA;

    s.setPixelColor(serpIdx(colA, row), CLR_RED);
    s.setPixelColor(serpIdx(colB, row), CLR_RED);

    // Thicken diagonals by one pixel
    if (colA > 0)              s.setPixelColor(serpIdx(colA - 1, row), 0x660000);
    if (colA < STRIP_COLS - 1) s.setPixelColor(serpIdx(colA + 1, row), 0x660000);
    if (colB > 0)              s.setPixelColor(serpIdx(colB - 1, row), 0x660000);
    if (colB < STRIP_COLS - 1) s.setPixelColor(serpIdx(colB + 1, row), 0x660000);
  }
  s.show();
}

// ── Startup light sequence ────────────────────────────────────────────────────
void doStartupSequence() {
  for (int i = 0; i < 2; i++) {
    setAllMosfets(200); delay(160);
    setAllMosfets(0);   delay(130);
  }

  const uint32_t colours[] = {
    strip1.Color(255,  40,   0),   // orange
    strip1.Color(  0, 255,  70),   // green
    strip1.Color(120,   0, 255),   // purple
    strip1.Color(  0,  80, 255),   // blue
    strip1.Color(255, 255, 255),   // white
  };
  for (uint32_t c : colours) {
    fillStrip(strip1, c);
    fillStrip(strip2, c);
    delay(180);
  }

  fillStrip(strip1, CLR_OFF);
  fillStrip(strip2, CLR_OFF);
}
