#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// ===== Parámetros ojos =====
const int EYE_W = 60;
const int EYE_H_NORMAL = 80;
const int EYE_H_SHRINK = 70;
const int RADIUS = 15;
const int GAP = 15;

const uint16_t BG_COLOR = TFT_BLACK;
const uint16_t EYE_FILL = TFT_CYAN;

// ===== Movimiento =====
const int MOVE_AMPLITUDE_X = 50;
const unsigned long MOVE_TIME_MS = 150;   // ida/vuelta
const unsigned long HOLD_TIME_MS = 2000;  // espera en el extremo

// ===== Parpadeo normal (tapas rectas) =====
enum BlinkState { BOPEN, BCLOSING, BCLOSED, BOPENING };
BlinkState blinkState = BOPEN;
unsigned long blinkStateStartMs = 0;
unsigned long lastBlinkIntervalStart = 0;
unsigned long BLINK_INTERVAL_MS = 2500;
const unsigned long CLOSE_TIME_MS  = 120;
const unsigned long CLOSED_HOLD_MS =  60;
const unsigned long OPEN_TIME_MS   = 120;
float lidProgress = 0.0f;

// ===== Movimiento con periodo de 10 s =====
enum MoveState {
  IDLE,
  MOVE_RIGHT_OUT, RIGHT_HOLD, MOVE_RIGHT_BACK,
  MOVE_LEFT_OUT,  LEFT_HOLD,  MOVE_LEFT_BACK
};
MoveState moveState = IDLE;
unsigned long moveStateStartMs = 0;
unsigned long nextMoveTriggerMs = 0;   // cada 10 s
bool nextDirectionRight = true;

int moveOffsetX = 0;
int leftEyeH  = EYE_H_NORMAL;
int rightEyeH = EYE_H_NORMAL;

// ===== Canvas parcial 8-bit =====
const int BASE_W = (EYE_W * 2) + GAP;   // 135
const int PAD    = 6;
const int STAGE_W = BASE_W + (MOVE_AMPLITUDE_X * 2) + PAD * 2; // 247
const int STAGE_H = EYE_H_NORMAL + PAD * 2;                    // 92
TFT_eSprite stage = TFT_eSprite(&tft);

// ===== Modo ENOJADO (se mantiene) =====
const unsigned long ANGRY_DELAY_MS    = 5000;  // a los 5 s
const unsigned long ANGRY_DURATION_MS = 2000;  // 2 s
bool angryActive  = false;
bool angryPlayed  = false;
unsigned long angryStartMs = 0;

// ===== Secuencia de “despertar” (NUEVA, se agrega) =====
enum WakeState {
  ASLEEP_INIT, ASLEEP_HOLD,
  WAKE_OPENING,
  WAKE_BLINK1_CLOSE, WAKE_BLINK1_OPEN,
  WAKE_BLINK2_CLOSE, WAKE_BLINK2_OPEN,
  WAKE_DONE
};
WakeState wakeState = ASLEEP_INIT;

unsigned long wakeStateStartMs = 0;
const float   SLEEP_LID_LEVEL = 0.95f;     // 95% cerrado
const unsigned long SLEEP_TIME_MS   = 3000; // 3 s dormido
const unsigned long WAKE_OPEN_TIME_MS  = 180;
const unsigned long WAKE_CLOSE_TIME_MS = 100;
const unsigned long WAKE_OPEN2_TIME_MS = 120;

// ---------- Utils ----------
static inline float easeInOutQuad(float x) {
  return (x < 0.5f) ? (2.0f*x*x)
                    : (1.0f - ((-2.0f*x + 2.0f)*(-2.0f*x + 2.0f))/2.0f);
}
int lerpInt(int a, int b, float t) {
  if (t < 0) t = 0; if (t > 1) t = 1;
  return a + (int)((b - a) * t);
}
float clamp01(float v){ return v<0?0:(v>1?1:v); }

// ---------- Ojo NORMAL (sin pupila) ----------
void drawEyeNormal(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int halfH = h / 2;
  int lidH  = (int)(lidProgress * halfH);
  if (lidH > 0) {
    stage.fillRect(x, y, w, lidH, BG_COLOR);             // tapa superior recta
    stage.fillRect(x, y + h - lidH, w, lidH, BG_COLOR);  // tapa inferior recta
  }
}

// ---------- Ojo ENOJADO (sin cejas rojas) ----------
void drawEyeAngryLeft(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  // lado interno = derecha
  stage.fillTriangle(x, y, x + w, y, x + w, y + tilt, BG_COLOR);
}
void drawEyeAngryRight(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  // lado interno = izquierda
  stage.fillTriangle(x, y + tilt, x, y, x + w, y, BG_COLOR);
}

// ---------- Render escena ----------
void renderScene() {
  stage.fillSprite(BG_COLOR);

  int centerInStage = STAGE_W / 2;
  int baseLeftX  = centerInStage - (BASE_W / 2) + moveOffsetX;
  int baseRightX = baseLeftX + EYE_W + GAP;
  int leftY  = (STAGE_H - leftEyeH)  / 2;
  int rightY = (STAGE_H - rightEyeH) / 2;

  if (angryActive) {
    drawEyeAngryLeft(baseLeftX,  leftY,  EYE_W, leftEyeH);
    drawEyeAngryRight(baseRightX, rightY, EYE_W, rightEyeH);
  } else {
    drawEyeNormal(baseLeftX,  leftY,  EYE_W, leftEyeH);
    drawEyeNormal(baseRightX, rightY, EYE_W, rightEyeH);
  }

  int screenX = (tft.width()  - STAGE_W) / 2;
  int screenY = (tft.height() - STAGE_H) / 2;
  stage.pushSprite(screenX, screenY);
}

// ---------- Parpadeo normal ----------
void updateBlink() {
  if (angryActive) return;  // mientras está enojado, pausa parpadeo

  unsigned long now = millis();
  unsigned long elapsed = now - blinkStateStartMs;

  switch (blinkState) {
    case BOPEN:
      lidProgress = 0.0f;
      if (now - lastBlinkIntervalStart >= BLINK_INTERVAL_MS) {
        blinkState = BCLOSING; blinkStateStartMs = now;
      }
      break;
    case BCLOSING: {
      float p = (float)elapsed / (float)CLOSE_TIME_MS;
      if (p >= 1.0f) { lidProgress = 1.0f; blinkState = BCLOSED; blinkStateStartMs = now; }
      else           { lidProgress = easeInOutQuad(p); }
    } break;
    case BCLOSED:
      if (elapsed >= CLOSED_HOLD_MS) { blinkState = BOPENING; blinkStateStartMs = now; }
      break;
    case BOPENING: {
      float p = (float)elapsed / (float)OPEN_TIME_MS;
      if (p >= 1.0f) { lidProgress = 0.0f; blinkState = BOPEN; lastBlinkIntervalStart = now; }
      else           { lidProgress = 1.0f - easeInOutQuad(p); }
    } break;
  }
}

// ---------- Movimiento (periodo 10 s, ojo de la dirección se achica) ----------
void updateMove() {
  unsigned long now = millis();
  unsigned long elapsed = now - moveStateStartMs;

  switch (moveState) {
    case IDLE:
      moveOffsetX = 0;
      leftEyeH  = EYE_H_NORMAL;
      rightEyeH = EYE_H_NORMAL;
      if (now >= nextMoveTriggerMs) {
        moveState = nextDirectionRight ? MOVE_RIGHT_OUT : MOVE_LEFT_OUT;
        moveStateStartMs = now;
      }
      break;

    case MOVE_RIGHT_OUT: {
      float p = (float)elapsed / MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = +MOVE_AMPLITUDE_X;
        rightEyeH = EYE_H_SHRINK;
        moveState = RIGHT_HOLD;
        moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = (int)(+MOVE_AMPLITUDE_X * e);
        rightEyeH = lerpInt(EYE_H_NORMAL, EYE_H_SHRINK, e);
        leftEyeH  = EYE_H_NORMAL;
      }
    } break;

    case RIGHT_HOLD:
      moveOffsetX = +MOVE_AMPLITUDE_X;
      rightEyeH = EYE_H_SHRINK;
      if (elapsed >= HOLD_TIME_MS) {
        moveState = MOVE_RIGHT_BACK; moveStateStartMs = now;
      }
    } break;

    case LEFT_HOLD:
      moveOffsetX = -MOVE_AMPLITUDE_X;
      leftEyeH = EYE_H_SHRINK;
      if (elapsed >= HOLD_TIME_MS) { moveState = MOVE_LEFT_BACK; moveStateStartMs = now; }
      break;

    case MOVE_RIGHT_BACK: {
      float p = (float)elapsed / MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = 0;
        rightEyeH = EYE_H_NORMAL;
        nextDirectionRight = false;
        nextMoveTriggerMs = now + 10000UL;  // próximo en 10 s (izq)
        moveState = IDLE; moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = +MOVE_AMPLITUDE_X - (int)(+MOVE_AMPLITUDE_X * e);
        rightEyeH = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);
      }
    } break;

    case MOVE_LEFT_OUT: {
      float p = (float)elapsed / MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = -MOVE_AMPLITUDE_X;
        leftEyeH = EYE_H_SHRINK;
        moveState = LEFT_HOLD;
        moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = -(int)(+MOVE_AMPLITUDE_X * e);
        leftEyeH  = lerpInt(EYE_H_NORMAL, EYE_H_SHRINK, e);
        rightEyeH = EYE_H_NORMAL;
      }
    } break;

    case LEFT_HOLD:
      moveOffsetX = -MOVE_AMPLITUDE_X;
      leftEyeH = EYE_H_SHRINK;
      if (elapsed >= HOLD_TIME_MS) {
        moveState = MOVE_LEFT_BACK; moveStateStartMs = now;
      }
      break;

    case MOVE_LEFT_BACK: {
      float p = (float)elapsed / MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = 0;
        leftEyeH = EYE_H_NORMAL;
        nextDirectionRight = true;
        nextMoveTriggerMs = now + 10000UL;  // próximo en 10 s (der)
        moveState = IDLE; moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = -MOVE_AMPLITUDE_X + (int)(+MOVE_AMPLITUDE_X * e);
        leftEyeH = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);
      }
    } break;
  }
}

// ---------- Angry scheduler (se mantiene) ----------
void updateAngry() {
  unsigned long now = millis();
  if (!angryPlayed && !angryActive && now >= ANGRY_DELAY_MS) {
    angryActive = true;
    angryStartMs = now;
  }
  if (angryActive && (now - angryStartMs >= ANGRY_DURATION_MS)) {
    angryActive = false;
    angryPlayed = true; // una sola vez
  }
}

// ---------- Wake sequence (NUEVO, se agrega) ----------
void updateWakeSequence() {
  unsigned long now = millis();
  unsigned long elapsed = now - wakeStateStartMs;

  switch (wakeState) {
    case ASLEEP_INIT:
      // Arranca dormido al 95%
      lidProgress = SLEEP_LID_LEVEL;
      // Pausar movimiento hasta terminar el wake
      moveState = IDLE;
      nextMoveTriggerMs = now + 100000000UL;
      wakeState = ASLEEP_HOLD;
      wakeStateStartMs = now;
      break;

    case ASLEEP_HOLD:
      lidProgress = SLEEP_LID_LEVEL;
      if (elapsed >= SLEEP_TIME_MS) {
        wakeState = WAKE_OPENING; wakeStateStartMs = now;
      }
      break;

    case WAKE_OPENING: {
      float p = clamp01((float)elapsed / (float)WAKE_OPEN_TIME_MS);
      lidProgress = (1.0f - p) * SLEEP_LID_LEVEL; // 0.95 -> 0
      if (p >= 1.0f) { wakeState = WAKE_BLINK1_CLOSE; wakeStateStartMs = now; }
    } break;

    case WAKE_BLINK1_CLOSE: {
      float p = clamp01((float)elapsed / (float)WAKE_CLOSE_TIME_MS);
      lidProgress = p;
      if (p >= 1.0f) { wakeState = WAKE_BLINK1_OPEN; wakeStateStartMs = now; }
    } break;

    case WAKE_BLINK1_OPEN: {
      float p = clamp01((float)elapsed / (float)WAKE_OPEN2_TIME_MS);
      lidProgress = 1.0f - p;
      if (p >= 1.0f) { wakeState = WAKE_BLINK2_CLOSE; wakeStateStartMs = now; }
    } break;

    case WAKE_BLINK2_CLOSE: {
      float p = clamp01((float)elapsed / (float)WAKE_CLOSE_TIME_MS);
      lidProgress = p;
      if (p >= 1.0f) { wakeState = WAKE_BLINK2_OPEN; wakeStateStartMs = now; }
    } break;

    case WAKE_BLINK2_OPEN: {
      float p = clamp01((float)elapsed / (float)WAKE_OPEN2_TIME_MS);
      lidProgress = 1.0f - p;
      if (p >= 1.0f) {
        // listo: habilitar parpadeo normal y programar movimiento
        blinkState = BOPEN;
        blinkStateStartMs = now;
        lastBlinkIntervalStart = now;

        nextDirectionRight = true;
        nextMoveTriggerMs = now + 10000UL; // primer movimiento en 10 s
        moveState = IDLE;

        wakeState = WAKE_DONE; wakeStateStartMs = now;
      }
    } break;

    case WAKE_DONE:
      // nada: deja que blink/move/angry sigan
      break;
  }
}

// ---------- Init ----------
void initScene() {
  tft.fillScreen(BG_COLOR);

  stage.setColorDepth(8);
  stage.createSprite(STAGE_W, STAGE_H);

  unsigned long now = millis();

  // Parpadeo base
  blinkState = BOPEN;
  blinkStateStartMs = now;
  lastBlinkIntervalStart = now;

  // Movimiento (lo posponemos hasta finalizar wake)
  moveState = IDLE;
  moveStateStartMs = now;
  nextDirectionRight = true;
  nextMoveTriggerMs = now + 100000000UL;

  // Angry (se mantiene)
  angryActive = false;
  angryPlayed = false;

  // Wake (nuevo)
  wakeState = ASLEEP_INIT;
  wakeStateStartMs = now;

  renderScene();
}

void setup() {
  tft.init();
  tft.setRotation(0);
  initScene();
}

void loop() {
  // Primero la secuencia de despertar (NUEVA)
  if (wakeState != WAKE_DONE) {
    updateWakeSequence();
  } else {
    // Parpadeo y movimiento normales
    updateBlink();
    updateMove();
  }

  // Angry corre SIEMPRE (se superpone; pausa blink cuando activo)
  updateAngry();

  // Un solo push por frame
  renderScene();
}
