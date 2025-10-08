#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// ===== Parámetros ojos =====
const int EYE_W = 60;
const int EYE_H_NORMAL = 80;
const int EYE_H_SHRINK = 60;
const int RADIUS = 10;
const int GAP = 15;

const uint16_t BG_COLOR = TFT_BLACK;
const uint16_t EYE_FILL = TFT_CYAN;

// ===== Movimiento =====
const int MOVE_AMPLITUDE_X = 50;            // ±60 px
const unsigned long MOVE_TIME_MS = 150;     // 150 ms por tramo
const unsigned long HOLD_TIME_MS = 2000;    // 2 s de espera

// ===== Parpadeo (tapas rectas, cierre al centro) =====
enum BlinkState { BOPEN, BCLOSING, BCLOSED, BOPENING };
BlinkState blinkState = BOPEN;
unsigned long blinkStateStartMs = 0;
unsigned long lastBlinkIntervalStart = 0;
unsigned long BLINK_INTERVAL_MS = 2500;
const unsigned long CLOSE_TIME_MS  = 120;
const unsigned long CLOSED_HOLD_MS =  60;
const unsigned long OPEN_TIME_MS   = 120;
float lidProgress = 0.0f;

// ===== Estado de movimiento y scheduler =====
enum MoveState {
  IDLE,
  MOVE_RIGHT_OUT, RIGHT_HOLD, MOVE_RIGHT_BACK,
  MOVE_LEFT_OUT,  LEFT_HOLD,  MOVE_LEFT_BACK
};
MoveState moveState = IDLE;

unsigned long moveStateStartMs = 0;
unsigned long nextMoveTriggerMs = 0;   // <<< cada 10 s
bool nextDirectionRight = true;        // alterna derecha/izquierda

int moveOffsetX = 0;                   // 0 centro, +der, -izq
int leftEyeH  = EYE_H_NORMAL;          // altura actual
int rightEyeH = EYE_H_NORMAL;

// ===== Canvas parcial (“stage”) 8-bit =====
const int BASE_W = (EYE_W * 2) + GAP;  // 135
const int PAD    = 4;
const int STAGE_W = BASE_W + (MOVE_AMPLITUDE_X * 2) + PAD * 2; // 263
const int STAGE_H = EYE_H_NORMAL + PAD * 2;                    // 88
TFT_eSprite stage = TFT_eSprite(&tft);

// ---------- Utils ----------
static inline float easeInOutQuad(float x) {
  return (x < 0.5f) ? (2.0f*x*x) : (1.0f - ((-2.0f*x + 2.0f)*(-2.0f*x + 2.0f))/2.0f);
}
int lerpInt(int a, int b, float t) {
  if (t < 0) t = 0; if (t > 1) t = 1;
  return a + (int)((b - a) * t);
}

// ---------- Dibujo de un ojo dentro del stage ----------
void drawEye(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int halfH = h / 2;
  int lidH  = (int)(lidProgress * halfH);
  if (lidH > 0) {
    stage.fillRect(x, y, w, lidH, BG_COLOR);            // tapa superior recta
    stage.fillRect(x, y + h - lidH, w, lidH, BG_COLOR); // tapa inferior recta
  }
}

// ---------- Render escena (sin flicker) ----------
void renderScene() {
  stage.fillSprite(BG_COLOR);

  int centerInStage = STAGE_W / 2;
  int baseLeftX  = centerInStage - (BASE_W / 2) + moveOffsetX;
  int baseRightX = baseLeftX + EYE_W + GAP;

  int leftY  = (STAGE_H - leftEyeH)  / 2;
  int rightY = (STAGE_H - rightEyeH) / 2;

  drawEye(baseLeftX,  leftY,  EYE_W, leftEyeH);
  drawEye(baseRightX, rightY, EYE_W, rightEyeH);

  int screenX = (tft.width()  - STAGE_W) / 2;
  int screenY = (tft.height() - STAGE_H) / 2;
  stage.pushSprite(screenX, screenY);
}

// ---------- Parpadeo ----------
void updateBlink() {
  unsigned long now = millis();
  unsigned long elapsed = now - blinkStateStartMs;

  switch (blinkState) {
    case BOPEN:
      lidProgress = 0.0f;
      if (now - lastBlinkIntervalStart >= BLINK_INTERVAL_MS) {
        blinkState = BCLOSING;
        blinkStateStartMs = now;
      }
      break;

    case BCLOSING: {
      float p = (float)elapsed / (float)CLOSE_TIME_MS;
      if (p >= 1.0f) { lidProgress = 1.0f; blinkState = BCLOSED; blinkStateStartMs = now; }
      else           { lidProgress = easeInOutQuad(p); }
    } break;

    case BCLOSED:
      lidProgress = 1.0f;
      if (elapsed >= CLOSED_HOLD_MS) { blinkState = BOPENING; blinkStateStartMs = now; }
      break;

    case BOPENING: {
      float p = (float)elapsed / (float)OPEN_TIME_MS;
      if (p >= 1.0f) { lidProgress = 0.0f; blinkState = BOPEN; lastBlinkIntervalStart = now; }
      else           { lidProgress = 1.0f - easeInOutQuad(p); }
    } break;
  }
}

// ---------- Movimiento con período de 10 s ----------
void updateMove() {
  unsigned long now = millis();
  unsigned long elapsed = now - moveStateStartMs;

  switch (moveState) {
    case IDLE:
      // centro, tamaños normales
      moveOffsetX = 0;
      leftEyeH  = EYE_H_NORMAL;
      rightEyeH = EYE_H_NORMAL;

      // esperar al próximo disparo (cada 10 s)
      if (now >= nextMoveTriggerMs) {
        if (nextDirectionRight) { moveState = MOVE_RIGHT_OUT; }
        else                    { moveState = MOVE_LEFT_OUT;  }
        moveStateStartMs = now;
      }
      break;

    case MOVE_RIGHT_OUT: {
      float p = (float)elapsed / (float)MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = +MOVE_AMPLITUDE_X;
        rightEyeH = EYE_H_SHRINK;
        moveState = RIGHT_HOLD;
        moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = (int)(+MOVE_AMPLITUDE_X * e);
        rightEyeH = lerpInt(EYE_H_NORMAL, EYE_H_SHRINK, e); // se achica mientras se mueve
        leftEyeH  = EYE_H_NORMAL;
      }
    } break;

    case RIGHT_HOLD:
      moveOffsetX = +MOVE_AMPLITUDE_X;
      rightEyeH = EYE_H_SHRINK;
      leftEyeH  = EYE_H_NORMAL;
      if (elapsed >= HOLD_TIME_MS) { moveState = MOVE_RIGHT_BACK; moveStateStartMs = now; }
      break;

    case MOVE_RIGHT_BACK: {
      float p = (float)elapsed / (float)MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = 0;
        rightEyeH = EYE_H_NORMAL;       // vuelve a normal al centro
        // programar el próximo disparo en 10 s al lado contrario
        nextDirectionRight = false;
        nextMoveTriggerMs = now + 10000UL;
        moveState = IDLE;
        moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = +MOVE_AMPLITUDE_X - (int)(+MOVE_AMPLITUDE_X * e);
        rightEyeH = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);
      }
    } break;

    case MOVE_LEFT_OUT: {
      float p = (float)elapsed / (float)MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = -MOVE_AMPLITUDE_X;
        leftEyeH = EYE_H_SHRINK;
        moveState = LEFT_HOLD;
        moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = -(int)(+MOVE_AMPLITUDE_X * e);
        leftEyeH  = lerpInt(EYE_H_NORMAL, EYE_H_SHRINK, e); // se achica mientras se mueve
        rightEyeH = EYE_H_NORMAL;
      }
    } break;

    case LEFT_HOLD:
      moveOffsetX = -MOVE_AMPLITUDE_X;
      leftEyeH = EYE_H_SHRINK;
      rightEyeH = EYE_H_NORMAL;
      if (elapsed >= HOLD_TIME_MS) { moveState = MOVE_LEFT_BACK; moveStateStartMs = now; }
      break;

    case MOVE_LEFT_BACK: {
      float p = (float)elapsed / (float)MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = 0;
        leftEyeH = EYE_H_NORMAL;
        // programar el próximo disparo en 10 s al lado contrario
        nextDirectionRight = true;
        nextMoveTriggerMs = now + 10000UL;
        moveState = IDLE;
        moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = -MOVE_AMPLITUDE_X + (int)(+MOVE_AMPLITUDE_X * e);
        leftEyeH = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);
      }
    } break;
  }
}

// ---------- Init ----------
void initScene() {
  tft.fillScreen(BG_COLOR);

  stage.setColorDepth(8);                 // ahorro de RAM
  stage.createSprite(STAGE_W, STAGE_H);   // ~23 KB

  unsigned long now = millis();

  // Parpadeo
  blinkState = BOPEN;
  blinkStateStartMs = now;
  lastBlinkIntervalStart = now;

  // Movimiento
  moveState = IDLE;
  moveStateStartMs = now;
  nextDirectionRight = true;              // a los 10 s → derecha
  nextMoveTriggerMs = now + 10000UL;      // <<< primer disparo a los 10 s

  renderScene();
}

void setup() {
  tft.init();
  tft.setRotation(0);
  initScene();
}

void loop() {
  updateBlink();
  updateMove();
  renderScene();  // un solo push por frame
}
