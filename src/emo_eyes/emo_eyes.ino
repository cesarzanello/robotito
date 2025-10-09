#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>

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

// ===== Movimiento (periodo 10 s) =====
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

// ===== ENOJADO (se mantiene) =====
const unsigned long ANGRY_DELAY_MS    = 5000;  // a los 5 s
const unsigned long ANGRY_DURATION_MS = 2000;  // 2 s
bool angryActive  = false;
bool angryPlayed  = false;
unsigned long angryStartMs = 0;

// ===== TRISTE (nuevo) =====
enum SadState { SAD_IDLE, SAD_WAIT, SAD_RUN, SAD_DONE };
SadState sadState = SAD_IDLE;

const unsigned long SAD_DELAY_MS    = 19000; // a los 19 s
const unsigned long SAD_DURATION_MS = 2000;  // dura 2 s
unsigned long sadStartMs = 0;

// leve entornado mientras está triste
const float SAD_LID = 0.25f;


// ===== DESPERTAR (se mantiene) =====
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

// ===== RISA (se mantiene) =====
enum LaughState { LAUGH_IDLE, LAUGH_WAIT, LAUGH_RUN, LAUGH_DONE };
LaughState laughState = LAUGH_IDLE;
const unsigned long LAUGH_DELAY_MS    = 7000;  // a los 7 s
const unsigned long LAUGH_DURATION_MS = 1800;  // ~1.8 s
unsigned long laughStartMs = 0;
int laughOffsetX = 0, laughOffsetY = 0;

// ===== CONTENTO (NUEVO) =====
enum HappyState { HAPPY_IDLE, HAPPY_WAIT, HAPPY_RUN, HAPPY_DONE };
HappyState happyState = HAPPY_IDLE;
const unsigned long HAPPY_DELAY_MS    = 17000; // a los 15 s
const unsigned long HAPPY_DURATION_MS = 2000;  // 2 s
unsigned long happyStartMs = 0;
// ojo “contento”: leve entornado
const float HAPPY_LID = 0.38f;



// ====== PRE-SLEEP (nuevo) ======
enum PreSleepState {
  PS_IDLE, PS_YAWN, PS_BLINK_SET_START, PS_BLINK_CLOSE, PS_BLINK_OPEN,
  PS_WAIT_BETWEEN_SETS, PS_SLOW_FALL, PS_DONE
};
PreSleepState preSleepState = PS_IDLE;

bool preSleepActive = false;
unsigned long preSleepStateMs = 0;

// Timings
const unsigned long PRESLEEP_YAWN_MS        = 600;   // duración del bostezo
const unsigned long PRESLEEP_BLINK_CLOSE_MS = 90;    // cierre de cada blink
const unsigned long PRESLEEP_BLINK_OPEN_MS  = 120;   // apertura de cada blink
const unsigned long PRESLEEP_WAIT_MS        = 1000;  // 1 s entre sets
const unsigned long PRESLEEP_SLOWFALL_MS    = 1200;  // caída lenta a 95%

// Plan: sets de 1, luego 2, luego 3 blinks
int currentBlinkSet = 0;             // 0→1→2→3
int blinksRemainingInSet = 0;        // cuántos faltan en el set actual
bool blinkClosingPhase = true;       // fase del blink

// ---- Dibujo de OJO "BOSTEZO" (párpado inferior diagonal hacia el centro) ----
void drawEyeYawnLeft(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  // Izquierdo: el lado interno es la derecha (x+w) → recorte inferior más alto ahí
  stage.fillTriangle(x, y + h, x + w, y + h, x + w, y + h - tilt, BG_COLOR);
}

void drawEyeYawnRight(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  // Derecho: lado interno es la izquierda (x)
  stage.fillTriangle(x, y + h - tilt, x, y + h, x + w, y + h, BG_COLOR);
}
// ===== SLEEP a los 25 s (se mantiene) =====
bool sleepActive = false;
const unsigned long sleepAfterMs = 35000UL; // 25 s
unsigned long bootMs = 0;

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

// ---------- OJO NORMAL (sin pupila) ----------
void drawEyeNormal(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int halfH = h / 2;
  int lidH  = (int)(lidProgress * halfH);
  if (lidH > 0) {
    stage.fillRect(x, y, w, lidH, BG_COLOR);             // tapa superior
    stage.fillRect(x, y + h - lidH, w, lidH, BG_COLOR);  // tapa inferior
  }
}

// ---------- OJO ENOJADO (sin cejas) ----------
void drawEyeAngryLeft(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  // lado interno (centro) es la derecha
  stage.fillTriangle(x, y, x + w, y, x + w, y + tilt, BG_COLOR);
}
void drawEyeAngryRight(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  // lado interno (centro) es la izquierda
  stage.fillTriangle(x, y + tilt, x, y, x + w, y, BG_COLOR);
}

// ---------- OJO CONTENTO (NUEVO) ----------
// OJO IZQUIERDO CONTENTO (externo más abierto, “sonrisa” desde abajo)
// IZQUIERDO (más cerrado: sube más la esquina externa y recorta más arriba)
void drawEyeHappyLeft(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);

  // Recorte superior (más fuerte que antes)
  int upperInner = (int)(h * 0.14f);                       // antes ~0.10
  stage.fillTriangle(x + w, y,  x, y,  x, y + upperInner, BG_COLOR);

  // Párpado inferior “sonrisa” más alto (cierra más)
  int liftOuter = (int)(h * 0.40f);                        // antes ~0.28
  int liftInner = (int)(h * 0.22f);                        // antes ~0.12
  stage.fillTriangle(x,     y + h, x + w, y + h, x + w, y + h - liftInner, BG_COLOR);
  stage.fillTriangle(x,     y + h, x,     y + h - liftOuter, x + w, y + h - liftInner, BG_COLOR);
}

// DERECHO (espejo)
void drawEyeHappyRight(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);

  int upperInner = (int)(h * 0.14f);
  stage.fillTriangle(x, y + upperInner,  x, y,  x + w, y, BG_COLOR);

  int liftOuter = (int)(h * 0.40f);
  int liftInner = (int)(h * 0.22f);
  stage.fillTriangle(x,     y + h, x + w, y + h, x,     y + h - liftInner, BG_COLOR);
  stage.fillTriangle(x + w, y + h, x + w, y + h - liftOuter, x, y + h - liftInner, BG_COLOR);
}


// TRISTE
void drawEyeSadLeft(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);

  // Superior: baja más en la esquina externa (izquierda)
  int upperOuter = (int)(h * 0.16f);
  stage.fillTriangle(x, y + upperOuter,  x, y,  x + w, y, BG_COLOR);

  // Inferior: cae (más bajo) en la esquina externa y sube en la interna
  int dropOuter = (int)(h * 0.24f); // más bajo afuera
  int dropInner = (int)(h * 0.10f); // menos bajo adentro
  // Construimos el “arco” con dos triángulos
  stage.fillTriangle(x,     y + h, x + w, y + h, x,     y + h - dropOuter, BG_COLOR);
  stage.fillTriangle(x + w, y + h, x + w, y + h - dropInner, x, y + h - dropOuter, BG_COLOR);
}

void drawEyeSadRight(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);

  int upperOuter = (int)(h * 0.16f);
  // En el derecho, la esquina externa es la DERECHA (x+w)
  stage.fillTriangle(x + w, y + upperOuter,  x + w, y,  x, y, BG_COLOR);

  int dropOuter = (int)(h * 0.24f);
  int dropInner = (int)(h * 0.10f);
  stage.fillTriangle(x + w, y + h, x, y + h, x + w, y + h - dropOuter, BG_COLOR);
  stage.fillTriangle(x,     y + h, x, y + h - dropInner, x + w, y + h - dropOuter, BG_COLOR);
}



// ---------- Render escena (prioridad: angry > happy > normal) ----------
void renderScene() {
  stage.fillSprite(BG_COLOR);

  // centro + desplazamiento (y offsets de risa si aplica)
  int centerInStage = STAGE_W / 2;
  int baseLeftX  = centerInStage - (BASE_W / 2) + moveOffsetX + laughOffsetX;
  int baseRightX = baseLeftX + EYE_W + GAP;
  int leftY  = (STAGE_H - leftEyeH)  / 2 + laughOffsetY;
  int rightY = (STAGE_H - rightEyeH) / 2 + laughOffsetY;

  if (preSleepActive && preSleepState == PS_YAWN) {
    // BOSTEZO (párpado inferior diagonal hacia el centro)
    drawEyeYawnLeft(baseLeftX,  leftY,  EYE_W, leftEyeH);
    drawEyeYawnRight(baseRightX, rightY, EYE_W, rightEyeH);
  }
  else if (angryActive) {
    drawEyeAngryLeft(baseLeftX,  leftY,  EYE_W, leftEyeH);
    drawEyeAngryRight(baseRightX, rightY, EYE_W, rightEyeH);
  }
  else if (sadState == SAD_RUN) {
    drawEyeSadLeft(baseLeftX,  leftY,  EYE_W, leftEyeH);
    drawEyeSadRight(baseRightX, rightY, EYE_W, rightEyeH);
  }
  else if (happyState == HAPPY_RUN) {
    drawEyeHappyLeft(baseLeftX,  leftY,  EYE_W, leftEyeH);
    drawEyeHappyRight(baseRightX, rightY, EYE_W, rightEyeH);
  }
  else {
    drawEyeNormal(baseLeftX,  leftY,  EYE_W, leftEyeH);
    drawEyeNormal(baseRightX, rightY, EYE_W, rightEyeH);
  }


  int screenX = (tft.width()  - STAGE_W) / 2;
  int screenY = (tft.height() - STAGE_H) / 2;
  stage.pushSprite(screenX, screenY);
}

// ---------- Parpadeo normal ----------
void updateBlink() {
  // Pausar parpadeo si hay escenas que lo deben dominar
  if (preSleepActive || angryActive || laughState == LAUGH_RUN
    || happyState == HAPPY_RUN || sadState == SAD_RUN) return;


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
      break;

    case MOVE_RIGHT_BACK: {
      float p = (float)elapsed / MOVE_TIME_MS;
      if (p >= 1.0f) {
        moveOffsetX = 0;
        rightEyeH = EYE_H_NORMAL;
        nextDirectionRight = false;
        nextMoveTriggerMs = now + 10000UL;
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
        nextMoveTriggerMs = now + 10000UL;
        moveState = IDLE; moveStateStartMs = now;
      } else {
        float e = easeInOutQuad(p);
        moveOffsetX = -MOVE_AMPLITUDE_X + (int)(+MOVE_AMPLITUDE_X * e);
        leftEyeH = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);
      }
    } break;
  }
}

// ---------- ENOJADO ----------
void updateAngry() {
  unsigned long now = millis();
  if (!angryPlayed && !angryActive && now >= ANGRY_DELAY_MS && !sleepActive) {
    angryActive = true;
    angryStartMs = now;
  }
  if (angryActive && (now - angryStartMs >= ANGRY_DURATION_MS)) {
    angryActive = false;
    angryPlayed = true; // una sola vez
  }
}

// ---------- RISA ----------
void updateLaugh() {
  unsigned long now = millis();

  switch (laughState) {
    case LAUGH_IDLE:
      laughOffsetX = laughOffsetY = 0;
      break;

    case LAUGH_WAIT:
      if (now >= LAUGH_DELAY_MS && !angryActive && !sleepActive) {
        laughState = LAUGH_RUN; laughStartMs = now;
      }
      break;

    case LAUGH_RUN: {
      unsigned long t = now - laughStartMs;
      if (t >= LAUGH_DURATION_MS || angryActive || sleepActive) {
        laughState = LAUGH_DONE;
        laughOffsetX = laughOffsetY = 0;
        lidProgress = 0.0f;
        break;
      }
      // micro-parpadeo y temblor
      float phase = (t % 125UL) / 125.0f;
      float s = (sinf(phase * 2.0f * 3.1415926f) * 0.5f + 0.5f);
      lidProgress = 0.6f * s;
      laughOffsetX = (int)(5.0f * sinf(t / 80.0f));
      laughOffsetY = (int)(3.0f * sinf(t / 110.0f));
    } break;

    case LAUGH_DONE:
      break;
  }
}

// ---------- CONTENTO (NUEVO) ----------
void updateHappy() {
  unsigned long now = millis();
  switch (happyState) {
    case HAPPY_IDLE:
      happyState = HAPPY_WAIT; // arma el reloj desde el arranque
      break;

    case HAPPY_WAIT:
      if (now >= HAPPY_DELAY_MS && !angryActive && !sleepActive) {
        happyState = HAPPY_RUN; happyStartMs = now;
      }
      break;

    case HAPPY_RUN:
      // dura fijo; pausa blink en updateBlink(); lidProgress lo manejan drawEyeHappy*
      if (now - happyStartMs >= HAPPY_DURATION_MS) {
        happyState = HAPPY_DONE;
        lidProgress = 0.0f; // volver a abierto
      } else {
        // opcional: pequeño “upbeat” del párpado (suave)
        lidProgress = HAPPY_LID; 
      }
      break;

    case HAPPY_DONE:
      // No repetimos; si querés repetir, rearmamos a HAPPY_WAIT con un nuevo timer
      break;
  }
}

// ---------TRISTE--------------
void updateSad() {
  unsigned long now = millis();

  switch (sadState) {
    case SAD_IDLE:
      sadState = SAD_WAIT; // arrancar reloj desde boot
      break;

    case SAD_WAIT:
      if (now >= SAD_DELAY_MS && !angryActive && !sleepActive) {
        sadState = SAD_RUN;
        sadStartMs = now;
      }
      break;

    case SAD_RUN:
      // Pausamos parpadeo con este flag (ver updateBlink)
      lidProgress = SAD_LID; // leve entornado constante
      if ((now - sadStartMs) >= SAD_DURATION_MS || angryActive || sleepActive) {
        sadState = SAD_DONE;
        lidProgress = 0.0f; // volver abierto
      }
      break;

    case SAD_DONE:
      // No se repite; si querés que vuelva, rearmamos a SAD_WAIT más adelante
      break;
  }
}

void updatePreSleep() {
  if (!preSleepActive) return;

  unsigned long now = millis();
  unsigned long elapsed = now - preSleepStateMs;

  switch (preSleepState) {

    case PS_YAWN:
      // solo dibujo especial en render; lidProgress no se usa acá
      if (elapsed >= PRESLEEP_YAWN_MS) {
        preSleepState = PS_BLINK_SET_START;
        preSleepStateMs = now;
        currentBlinkSet = 1;
      }
      break;

    case PS_BLINK_SET_START:
      blinksRemainingInSet = currentBlinkSet;
      blinkClosingPhase = true;
      preSleepState = PS_BLINK_CLOSE;
      preSleepStateMs = now;
      break;

    case PS_BLINK_CLOSE:
      // cerrar ojo (0 -> 1)
      if (elapsed >= PRESLEEP_BLINK_CLOSE_MS) {
        lidProgress = 1.0f;
        blinkClosingPhase = false;
        preSleepState = PS_BLINK_OPEN;
        preSleepStateMs = now;
      } else {
        float p = (float)elapsed / (float)PRESLEEP_BLINK_CLOSE_MS;
        lidProgress = p;
      }
      break;

    case PS_BLINK_OPEN:
      // abrir ojo (1 -> 0)
      if (elapsed >= PRESLEEP_BLINK_OPEN_MS) {
        lidProgress = 0.0f;
        blinksRemainingInSet--;
        if (blinksRemainingInSet > 0) {
          // siguiente blink del mismo set
          preSleepState = PS_BLINK_CLOSE;
          preSleepStateMs = now;
          blinkClosingPhase = true;
        } else {
          // set terminado → ¿hay más sets?
          if (currentBlinkSet < 3) {
            preSleepState = PS_WAIT_BETWEEN_SETS;  // esperar 1 s
            preSleepStateMs = now;
          } else {
            // listo → caída lenta
            preSleepState = PS_SLOW_FALL;
            preSleepStateMs = now;
          }
        }
      } else {
        float p = (float)elapsed / (float)PRESLEEP_BLINK_OPEN_MS;
        lidProgress = 1.0f - p;
      }
      break;

    case PS_WAIT_BETWEEN_SETS:
      lidProgress = 0.0f; // descansando abierto
      if (elapsed >= PRESLEEP_WAIT_MS) {
        currentBlinkSet++;
        preSleepState = PS_BLINK_SET_START;
        preSleepStateMs = now;
      }
      break;

    case PS_SLOW_FALL: {
      // 0 -> 0.95 suavemente
      if (elapsed >= PRESLEEP_SLOWFALL_MS) {
        lidProgress = 0.95f;
        preSleepState = PS_DONE;
        preSleepStateMs = now;

        // Entrar al sleep real y quedarse así
        enterSleep();           // tu función existente
      } else {
        float p = (float)elapsed / (float)PRESLEEP_SLOWFALL_MS;
        lidProgress = 0.95f * p;
      }
    } break;

    case PS_DONE:
      // ya estamos en sleepActive por enterSleep(); la pre–siesta terminó
      preSleepActive = false;
      break;
  }
}

// ---------- SLEEP (25 s) ----------
void enterSleep() {
  sleepActive = true;
  lidProgress = 0.95f;     // 95% cerrado
  moveState = IDLE;
  nextMoveTriggerMs = millis() + 100000000UL; // patear movimiento
}
void updateSleepScheduler() {
  if (!sleepActive && !preSleepActive && (millis() - bootMs >= sleepAfterMs)) {
    // Arrancar PRE-SLEEP
    preSleepActive = true;
    preSleepState = PS_YAWN;
    preSleepStateMs = millis();

    // Pausar movimiento y parpadeo normal durante la pre–siesta
    moveState = IDLE;
    nextMoveTriggerMs = millis() + 100000000UL;
  }
}


// ---------- WAKE (se mantiene) ----------
void updateWakeSequence() {
  unsigned long now = millis();
  unsigned long elapsed = now - wakeStateStartMs;

  switch (wakeState) {
    case ASLEEP_INIT:
      lidProgress = SLEEP_LID_LEVEL;         // 95% cerrado
      moveState = IDLE;                      // pausa movimiento
      nextMoveTriggerMs = now + 100000000UL; // pospuesto hasta terminar
      wakeState = ASLEEP_HOLD; wakeStateStartMs = now;
      laughState = LAUGH_WAIT;               // arranca reloj de risa
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
      break;
  }
}

// ---------- Init ----------
void initScene() {
  tft.fillScreen(BG_COLOR);

  stage.setColorDepth(8);
  stage.createSprite(STAGE_W, STAGE_H);

  unsigned long now = millis();
  bootMs = now;   // para SLEEP 25 s

  // Parpadeo base
  blinkState = BOPEN;
  blinkStateStartMs = now;
  lastBlinkIntervalStart = now;

  // Movimiento (pospuesto hasta finalizar wake)
  moveState = IDLE;
  moveStateStartMs = now;
  nextDirectionRight = true;
  nextMoveTriggerMs = now + 100000000UL;

  // Escenas
  angryActive = false; angryPlayed = false;
  laughState  = LAUGH_IDLE;
  happyState  = HAPPY_IDLE;
  sadState = SAD_IDLE;  // arranca el reloj de “triste”
  sleepActive = false;

  // Wake
  wakeState = ASLEEP_INIT;
  wakeStateStartMs = now;

  renderScene();
}

void setup() {
  tft.init();
  tft.setRotation(0);
  initScene();

  // Ejemplo para “despertar” con botón (opcional):
  // pinMode(0, INPUT_PULLUP);
}

void loop() {
  // 1) Programador de “sleep” a los 25 s → ahora arranca pre–siesta
  updateSleepScheduler();

  // 2) Si ya está dormido: mantener 95% y salir
  if (sleepActive) {
    renderScene();
    return;
  }

  // 3) Si estamos en pre–siesta: correrla y dibujar
  if (preSleepActive) {
    updatePreSleep();
    renderScene();
    return;
  }

  // 4) Flujo normal (cuando no hay sleep ni pre-sleep)
  if (wakeState != WAKE_DONE) {
    updateWakeSequence();
  } else {
    updateAngry();
    updateHappy();
    updateSad();
    updateLaugh();
    updateBlink();
    updateMove();
  }

  renderScene();
}

