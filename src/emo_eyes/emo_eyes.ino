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
const int GAP = 10;         // separación entre ojos

const uint16_t EYE_FILL   = TFT_CYAN;
const uint16_t EYE_BORDER = TFT_WHITE;
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

// Progreso del párpado (0.0 = abierto, 1.0 = cerrado)
float lidProgress = 0.0f;

// Dibuja el contenido de UN ojo dentro del sprite según el “progreso de párpado”
void renderEyeSprite(TFT_eSprite& spr) {
  // Limpiar sprite
  spr.fillSprite(BG_COLOR);

  // Ojo base (relleno + borde)
  spr.fillRoundRect(0, 0, EYE_W, EYE_H, RADIUS, EYE_FILL);
  spr.drawRoundRect(0, 0, EYE_W, EYE_H, RADIUS, EYE_BORDER);

  // Tapa (párpado) – cubre desde arriba hacia abajo
  int lidHeight = (int)(lidProgress * EYE_H);
  if (lidHeight > 0) {
    spr.fillRoundRect(0, 0, EYE_W, lidHeight, RADIUS, BG_COLOR);
    // Para que el borde superior quede “limpio”, se puede redibujar una línea
    // superior del borde si querés un look más marcado (opcional):
    // spr.drawRoundRect(0, 0, EYE_W, EYE_H, RADIUS, EYE_BORDER);
  }
}

// Vuelca ambos ojos a la pantalla
void pushEyes() {
  leftEye.pushSprite(leftX, eyesY);
  rightEye.pushSprite(rightX, eyesY);
}

// Llama para iniciar un parpadeo (cuando están abiertos)
void startBlink() {
  blinkState = CLOSING;
  stateStartMs = millis();
}

// Actualiza la máquina de estados del parpadeo (sin delays)
// Debe llamarse en loop()
void updateBlink() {
  unsigned long now = millis();
  unsigned long elapsed = now - stateStartMs;

  switch (blinkState) {
    case OPEN:
      // Espera hasta el próximo blink
      if (now - lastUpdateMs >= BLINK_INTERVAL_MS) {
        startBlink();
      }
      lidProgress = 0.0f;
      break;

    case CLOSING: {
      // Avanza de 0 → 1 en CLOSE_TIME_MS (lineal)
      float p = (float)elapsed / (float)CLOSE_TIME_MS;
      if (p >= 1.0f) {
        lidProgress = 1.0f;
        blinkState = CLOSED;
        stateStartMs = now;
      } else {
        lidProgress = p;
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
      // Vuelve de 1 → 0 en OPEN_TIME_MS (lineal)
      float p = (float)elapsed / (float)OPEN_TIME_MS;
      if (p >= 1.0f) {
        lidProgress = 0.0f;
        blinkState = OPEN;
        stateStartMs = now;
        lastUpdateMs = now; // reinicia intervalo
      } else {
        lidProgress = 1.0f - p;
      }
    } break;
  }

  // Redibujar sprites según progreso actual
  renderEyeSprite(leftEye);
  renderEyeSprite(rightEye);
  pushEyes();
}

/// ---------- Init de ojos y posiciones ----------
void initEyes() {
  // Crear los sprites
  leftEye.createSprite(EYE_W, EYE_H);
  rightEye.createSprite(EYE_W, EYE_H);

  // Calcular posiciones centradas (pantalla 240x240 por defecto)
  int W = tft.width();
  int H = tft.height();
  int totalWidth = (EYE_W * 2) + GAP;
  int startX = (W - totalWidth) / 2;
  int startY = (H - EYE_H) / 2;

  leftX  = startX;
  rightX = startX + EYE_W + GAP;
  eyesY  = startY;

  // Estado inicial: ojos abiertos renderizados una vez
  lidProgress = 0.0f;
  renderEyeSprite(leftEye);
  renderEyeSprite(rightEye);
  tft.fillScreen(BG_COLOR);
  pushEyes();

  // Inicializa temporizadores
  lastUpdateMs = millis();
  stateStartMs = millis();
  blinkState   = OPEN;
}

void setup() {
  tft.init();
  tft.setRotation(0);  // ajustá según tu montaje
  tft.fillScreen(BG_COLOR);

  initEyes();
}

void loop() {
  updateBlink();  // parpadeo no bloqueante
}
