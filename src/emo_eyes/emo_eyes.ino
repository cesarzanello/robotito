#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <math.h>

// ========================= Display configuration ==========================
// Update these pin assignments to match your wiring.
static constexpr int8_t TFT_SCK = 18;   // SPI clock
static constexpr int8_t TFT_MOSI = 23;  // SPI MOSI
static constexpr int8_t TFT_MISO = -1;  // Not used by most displays
static constexpr int8_t TFT_CS = 4;     // Chip select
static constexpr int8_t TFT_DC = 2;     // Data/command
static constexpr int8_t TFT_RST = 16;    // Reset pin (set to -1 if connected to ESP32 EN)
static constexpr int8_t TFT_BL = -1;    // Backlight control pin (set to -1 if tied to VCC)

// SPI bus and display driver objects for the GC9A01A.
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);
// ==========================================================================

// Eye geometry and colors.
static constexpr uint16_t COLOR_BACKGROUND = 0x0000; // Black
static constexpr uint16_t COLOR_EYE = 0x07FF;        // Cyan
static constexpr int16_t EYE_WIDTH = 60;
static constexpr int16_t EYE_HEIGHT = 80;
static constexpr int16_t EYE_PADDING = 10;

struct Eye {
  int16_t centerX;
  int16_t centerY;
};

Eye leftEye;
Eye rightEye;

// Blink animation timing (in milliseconds).
static constexpr uint32_t BLINK_INTERVAL_MIN = 2500;
static constexpr uint32_t BLINK_INTERVAL_MAX = 4500;
static constexpr uint32_t BLINK_DURATION = 200; // Time to close or open the eyelids

uint32_t nextBlinkAt = 0;
bool isBlinking = false;
bool eyesClosing = true;
uint32_t blinkPhaseStart = 0;
float eyelidProgress = 0.0f; // 0 = open, 1 = fully closed

// Forward declarations.
void scheduleNextBlink();
void drawEyes(float eyelidAmount);
void drawEye(const Eye &eye, float eyelidAmount);
void fillEllipse(int16_t centerX, int16_t centerY, int16_t width, int16_t height, uint16_t color);

void setup() {
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  }

  gfx->begin();
  gfx->fillScreen(COLOR_BACKGROUND);

  // Place the eyes roughly centered horizontally with a small gap.
  int16_t screenWidth = gfx->width();
  int16_t screenHeight = gfx->height();
  int16_t eyeOffsetX = (EYE_WIDTH / 2) + EYE_PADDING;

  leftEye.centerX = (screenWidth / 2) - eyeOffsetX;
  rightEye.centerX = (screenWidth / 2) + eyeOffsetX;
  leftEye.centerY = rightEye.centerY = screenHeight / 2;

  scheduleNextBlink();
  drawEyes(0.0f);
}

void loop() {
  uint32_t now = millis();

  if (!isBlinking && now >= nextBlinkAt) {
    isBlinking = true;
    eyesClosing = true;
    blinkPhaseStart = now;
  }

  if (isBlinking) {
    uint32_t phaseElapsed = now - blinkPhaseStart;
    float phaseProgress = constrain(static_cast<float>(phaseElapsed) / BLINK_DURATION, 0.0f, 1.0f);

    if (eyesClosing) {
      eyelidProgress = phaseProgress;
      if (phaseElapsed >= BLINK_DURATION) {
        // Switch to opening phase.
        eyesClosing = false;
        blinkPhaseStart = now;
      }
    } else {
      eyelidProgress = 1.0f - phaseProgress;
      if (phaseElapsed >= BLINK_DURATION) {
        isBlinking = false;
        eyelidProgress = 0.0f;
        scheduleNextBlink();
      }
    }
  }

  drawEyes(eyelidProgress);
  delay(16); // ~60 FPS refresh
}

void scheduleNextBlink() {
  uint32_t interval = random(BLINK_INTERVAL_MIN, BLINK_INTERVAL_MAX);
  nextBlinkAt = millis() + interval;
}

void drawEyes(float eyelidAmount) {
  drawEye(leftEye, eyelidAmount);
  drawEye(rightEye, eyelidAmount);
}

void drawEye(const Eye &eye, float eyelidAmount) {
  // Clear the eye region before redrawing to avoid ghosting.
  int16_t halfWidth = EYE_WIDTH / 2;
  int16_t halfHeight = EYE_HEIGHT / 2;
  gfx->fillRect(eye.centerX - halfWidth - 2, eye.centerY - halfHeight - 2,
                EYE_WIDTH + 4, EYE_HEIGHT + 4, COLOR_BACKGROUND);

  fillEllipse(eye.centerX, eye.centerY, EYE_WIDTH, EYE_HEIGHT, COLOR_EYE);

  if (eyelidAmount > 0.0f) {
    int16_t coverHeight = static_cast<int16_t>(EYE_HEIGHT * eyelidAmount * 0.5f);
    if (coverHeight > 0) {
      gfx->fillRect(eye.centerX - halfWidth, eye.centerY - halfHeight,
                    EYE_WIDTH, coverHeight, COLOR_BACKGROUND);
      gfx->fillRect(eye.centerX - halfWidth, eye.centerY + halfHeight - coverHeight,
                    EYE_WIDTH, coverHeight, COLOR_BACKGROUND);
    }
  }
}

void fillEllipse(int16_t centerX, int16_t centerY, int16_t width, int16_t height, uint16_t color) {
  float radiusX = width / 2.0f;
  float radiusY = height / 2.0f;
  float radiusXSquared = radiusX * radiusX;
  float radiusYSquared = radiusY * radiusY;

  for (int16_t y = -static_cast<int16_t>(radiusY); y <= static_cast<int16_t>(radiusY); ++y) {
    float normalizedY = static_cast<float>(y);
    float term = 1.0f - (normalizedY * normalizedY) / radiusYSquared;
    if (term < 0.0f) {
      continue;
    }
    float span = sqrtf(term * radiusXSquared);
    int16_t spanInt = static_cast<int16_t>(span + 0.5f);
    gfx->drawFastHLine(centerX - spanInt, centerY + y, spanInt * 2, color);
  }
}
