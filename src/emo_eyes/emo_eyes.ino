#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// Sprites individuales para cada ojo
TFT_eSprite leftEye  = TFT_eSprite(&tft);
TFT_eSprite rightEye = TFT_eSprite(&tft);

/// ---------- Parámetros de los ojos ----------
const int EYE_W = 60;       // ancho
const int EYE_H = 80;       // alto
const int RADIUS = 10;      // radio esquinas
const int GAP = 15;         // separación entre ojos

const uint16_t EYE_FILL   = TFT_CYAN;
const uint16_t BG_COLOR   = TFT_BLACK;

/// ---------- Posición en pantalla ----------
int leftX, rightX, eyesY;

/// ---------- Parpadeo (configurable) ----------
enum BlinkState { OPEN, CLOSING, CLOSED, OPENING };
BlinkState blinkState = OPEN;

unsigned long lastUpdateMs = 0;
unsigned long stateStartMs = 0;

const unsigned long BLINK_INTERVAL_MS = 2500;  // cada cuánto inicia un parpadeo
const unsigned long CLOSE_TIME_MS     = 120;   // duración del cierre
const unsigned long CLOSED_HOLD_MS    =  60;   // tiempo “cerrado”
const unsigned long OPEN_TIME_MS      = 120;   // duración de la apertura

// Progreso del párpado (0.0 = abierto, 1.0 = totalmente cerrado)
float lidProgress = 0.0f;

// Easing para suavizar el movimiento
static inline float easeInOutQuad(float x) {
  return (x < 0.5f) ? 2.0f*x*x : 1.0f - ((-2.0f*x + 2.0f)*(-2.0f*x + 2.0f))/2.0f;
}

// Dibuja UN ojo con cierre hacia el centro (sin bordes)
void renderEyeSprite(TFT_eSprite& spr) {
  spr.fillSprite(BG_COLOR);

  // Ojo base (relleno)
  spr.fillRoundRect(0, 0, EYE_W, EYE_H, RADIUS, EYE_FILL);

  // Altura de cada tapa (mitad superior e inferior)
  int halfH = EYE_H / 2;
  int lidH  = (int)(lidProgress * halfH);

  if (lidH > 0) {
    // Párpado superior que baja
    spr.fillRoundRect(0, 0, EYE_W, lidH, 0, BG_COLOR);
    // Párpado inferior que sube
    spr.fillRoundRect(0, EYE_H - lidH, EYE_W, lidH, 0, BG_COLOR);
  }
}

// Vuelca ambos ojos a la pantalla
void pushEyes() {
  leftEye.pushSprite(leftX, eyesY);
  rightEye.pushSprite(rightX, eyesY);
}

// Inicia el parpadeo
void startBlink() {
  blinkState = CLOSING;
  stateStartMs = millis();
}

// Actualiza la animación (sin delays)
void updateBlink() {
  unsigned long now = millis();
  unsigned long elapsed = now - stateStartMs;

  switch (blinkState) {
    case OPEN:
      lidProgress = 0.0f;
      if (now - lastUpdateMs >= BLINK_INTERVAL_MS) {
        startBlink();
      }
      break;

    case CLOSING: {
      float p = (float)elapsed / (float)CLOSE_TIME_MS;
      if (p >= 1.0f) {
        lidProgress = 1.0f;
        blinkState = CLOSED;
        stateStartMs = now;
      } else {
        lidProgress = easeInOutQuad(p);
      }
    } break;

    case CLOSED:
      lidProgress = 1.0f;
      if (elapsed >= CLOSED_HOLD_MS) {
        blinkState = OPENING;
        stateStartMs = now;
      }
      break;

    case OPENING: {
      float p = (float)elapsed / (float)OPEN_TIME_MS;
      if (p >= 1.0f) {
        lidProgress = 0.0f;
        blinkState = OPEN;
        stateStartMs = now;
        lastUpdateMs = now;
      } else {
        lidProgress = 1.0f - easeInOutQuad(p);
      }
    } break;
  }

  // Redibujar ambos ojos según progreso actual
  renderEyeSprite(leftEye);
  renderEyeSprite(rightEye);
  pushEyes();
}

/// ---------- Inicialización ----------
void initEyes() {
  leftEye.createSprite(EYE_W, EYE_H);
  rightEye.createSprite(EYE_W, EYE_H);

  int W = tft.width();
  int H = tft.height();
  int totalWidth = (EYE_W * 2) + GAP;
  int startX = (W - totalWidth) / 2;
  int startY = (H - EYE_H) / 2;

  leftX  = startX;
  rightX = startX + EYE_W + GAP;
  eyesY  = startY;

  lidProgress = 0.0f;
  tft.fillScreen(BG_COLOR);
  renderEyeSprite(leftEye);
  renderEyeSprite(rightEye);
  pushEyes();

  lastUpdateMs = millis();
  stateStartMs = millis();
  blinkState   = OPEN;
}

void setup() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(BG_COLOR);
  initEyes();
}

void loop() {
  updateBlink();
}
