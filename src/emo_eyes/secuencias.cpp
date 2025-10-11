#include <Arduino.h>
#include "secuencias.h"    // <- ya incluye estado.h
#include "constantes.h"
#include "funciones.h"
#include "escenas.h"
#include "ejecuciones.h"

// ====== Estado interno del motor ======
static Escena escenaActual = ESCENA_NINGUNA;

// --- Blink genérico (reutilizable) ---
static unsigned long blinkLastStart = 0;
static enum {B_IDLE, B_CLOSING, B_HOLD, B_OPENING} blinkState = B_IDLE;

static void blink_reset() {
  blinkState = B_IDLE;
  blinkLastStart = millis();
}
static void blink_force_open(EstadoOjos& est) {
  est.lidProgress = 0.0f;
  blink_reset();
}
static void blink_tick(EstadoOjos& est, unsigned long now, bool alwaysOn, unsigned long intervalMs) {
  if (blinkState == B_IDLE) {
    if (alwaysOn && (now - blinkLastStart >= intervalMs)) {
      blinkState = B_CLOSING; blinkLastStart = now;
    }
  } else if (blinkState == B_CLOSING) {
    float p = clamp01((float)(now - blinkLastStart) / BLINK_CLOSE_MS);
    est.lidProgress = p;
    if (p >= 1.0f) { blinkState = B_HOLD; blinkLastStart = now; }
  } else if (blinkState == B_HOLD) {
    est.lidProgress = 1.0f;
    if (now - blinkLastStart >= BLINK_HOLD_MS) {
      blinkState = B_OPENING; blinkLastStart = now;
    }
  } else if (blinkState == B_OPENING) {
    float p = clamp01((float)(now - blinkLastStart) / BLINK_OPEN_MS);
    est.lidProgress = 1.0f - p;
    if (p >= 1.0f) { blinkState = B_IDLE; blinkLastStart = now; }
  }
}
static bool blink_run_times(EstadoOjos& est, unsigned long now, int total, int& done, bool& running, unsigned long& phaseStart, int& phase) {
  // phase: 0 close, 1 hold, 2 open, repeat
  if (!running) { running = true; phase = 0; phaseStart = now; }
  if (phase == 0) {
    float p = clamp01((float)(now - phaseStart) / BLINK_CLOSE_MS);
    est.lidProgress = p;
    if (p >= 1.0f) { phase = 1; phaseStart = now; }
  } else if (phase == 1) {
    est.lidProgress = 1.0f;
    if (now - phaseStart >= BLINK_HOLD_MS) { phase = 2; phaseStart = now; }
  } else if (phase == 2) {
    float p = clamp01((float)(now - phaseStart) / BLINK_OPEN_MS);
    est.lidProgress = 1.0f - p;
    if (p >= 1.0f) {
      done++;
      if (done >= total) { running = false; return true; }
      phase = 0; phaseStart = now;
    }
  }
  return false; // no terminó aún
}

// --- Movimiento normal alterno ---
static unsigned long nextMoveAt = 0;
static bool moveRightNext = true;
static enum {MV_IDLE, MV_OUT, MV_HOLD, MV_BACK} moveState; // <-- agregado MV_HOLD
static unsigned long moveStart = 0;

static void move_reset(unsigned long now) {
  moveState = MV_IDLE;
  moveStart = now;
  nextMoveAt = now + NORMAL_MOVE_PERIOD_MS; // primera a los 10s
  moveRightNext = true;
}


// --- Emoción duración fija ---
static unsigned long emotionStart = 0;

// ====== Escena: PRE_DORMIR (fusionada) ======
static enum {
  PR_IDLE,
  PR_YAWN, PR_WAIT1, PR_BLINK1, PR_WAIT2, PR_BLINK2, PR_HALF_OPEN, PR_WAIT3,
  PR_BLINK3, PR_WAIT4, PR_SLOW_CLOSE, PR_DONE
} preState;
static unsigned long preStart = 0;
// blink por bloques
static bool preBlinkRunning = false; static int preBlinkPhase = 0, preBlinksDone = 0; static unsigned long preBlinkPhaseStart = 0;

static void pre_reset(unsigned long now) {
  preState = PR_YAWN; preStart = now;
  preBlinkRunning = false; preBlinkPhase = 0; preBlinksDone = 0; preBlinkPhaseStart = 0;
}

// ====== Escena: DESPERTAR ======
static enum {WK_IDLE, WK_CLOSED_HOLD, WK_BLINK3, WK_DONE} wakeState;
static unsigned long wakeStart = 0;
static bool wakeBlinkRunning = false; static int wakeBlinkPhase = 0, wakeBlinksDone = 0; static unsigned long wakeBlinkPhaseStart = 0;
// Timestamp de fin de DESPERTAR para disparar FELIZ a los 3s
static unsigned long wakeEndedAt = 0;

static void wake_reset(unsigned long now) {
  wakeState = WK_CLOSED_HOLD; wakeStart = now;
  wakeBlinkRunning = false; wakeBlinkPhase = 0; wakeBlinksDone = 0; wakeBlinkPhaseStart = 0;
}

// ====== Escena: NORMAL ======
static unsigned long normalStart = 0;

static void normal_reset(unsigned long now) {
  normalStart = now;
  blink_reset();
  move_reset(now);
}

// ====== API ======
void escenas_init(EstadoOjos& est) {
  escenaActual = ESCENA_NINGUNA;
  blink_reset();
  move_reset(millis());
  wakeEndedAt = 0;                // <- nuevo
  est.lidProgress = 0.0f;
  est.moveOffsetX = 0;
  est.leftEyeH = est.rightEyeH = EYE_H_NORMAL;
}

void escenas_set(Escena nueva, EstadoOjos& est) {
  unsigned long now = millis();
  escenaActual = nueva;

  // Si cambiamos a cualquier escena que NO sea NORMAL,
  // ya no queremos disparar FELIZ post-despertar.
  if (nueva != ESCENA_NORMAL) {
    wakeEndedAt = 0;  // <- nuevo
  }

  switch (nueva) {
    case ESCENA_PRE_DORMIR:
      pre_reset(now);
      est.escenaActual = ESCENA_PRE_SUENO;
      break;

    case ESCENA_DESPERTAR:
      wake_reset(now);
      est.escenaActual = ESCENA_DESPERTAR;
      est.lidProgress = 1.0f;
      break;

    case ESCENA_NORMAL:
      normal_reset(now);
      est.escenaActual = ESCENA_NORMAL;
      est.lidProgress = 0.0f;
      est.moveOffsetX = 0;
      est.leftEyeH = est.rightEyeH = EYE_H_NORMAL;
      // NOTA: wakeEndedAt se setea cuando terminamos DESPERTAR (más abajo)
      break;

    case ESCENA_ENOJADO:
    case ESCENA_FELIZ:
    case ESCENA_TRISTE:
    case ESCENA_RISA:
      emotionStart = now;
      est.escenaActual = (nueva == ESCENA_ENOJADO) ? ESCENA_ENOJADO
                         : (nueva == ESCENA_FELIZ) ? ESCENA_FELIZ
                         : (nueva == ESCENA_TRISTE) ? ESCENA_TRISTE
                         : ESCENA_RISA;
      break;

    default: break;
  }
}


Escena escenas_actual() { return escenaActual; }

// ====== UPDATE ======
void escenas_update(EstadoOjos& est, unsigned long now) {
  switch (escenaActual) {
    case ESCENA_PRE_DORMIR: {
      switch (preState) {
        case PR_YAWN:
          // Bostezo (forma especial) durante PRE_YAWN_MS
          est.escenaActual = ESCENA_PRE_SUENO; // tu render usa ojo bostezo
          if (now - preStart >= PRE_YAWN_MS) { preState = PR_WAIT1; preStart = now; }
          break;

        case PR_WAIT1:
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_BLINK1; preStart = now; }
          break;

        case PR_BLINK1:
          if (blink_run_times(est, now, 1, preBlinksDone, preBlinkRunning, preBlinkPhaseStart, preBlinkPhase)) {
            preBlinksDone = 0; preBlinkRunning = false; preState = PR_WAIT2; preStart = now;
          }
          est.escenaActual = ESCENA_NORMAL; // parpadeo en forma normal
          break;

        case PR_WAIT2:
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_BLINK2; preStart = now; }
          break;

        case PR_BLINK2:
          if (blink_run_times(est, now, 2, preBlinksDone, preBlinkRunning, preBlinkPhaseStart, preBlinkPhase)) {
            preBlinksDone = 0; preBlinkRunning = false; preState = PR_HALF_OPEN; preStart = now;
          }
          est.escenaActual = ESCENA_NORMAL;
          break;

        case PR_HALF_OPEN:
          est.escenaActual = ESCENA_NORMAL;
          est.lidProgress = 0.5f; // abre hasta la mitad
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_BLINK3; preStart = now; }
          break;

        case PR_BLINK3:
          if (blink_run_times(est, now, 3, preBlinksDone, preBlinkRunning, preBlinkPhaseStart, preBlinkPhase)) {
            preBlinksDone = 0; preBlinkRunning = false; preState = PR_WAIT4; preStart = now;
          }
          est.escenaActual = ESCENA_NORMAL;
          break;

        case PR_WAIT4:
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_SLOW_CLOSE; preStart = now; }
          break;

        case PR_SLOW_CLOSE: {
          // cierre muy despacio hasta SLEEP_LID_LEVEL
          float p = clamp01((float)(now - preStart) / PRE_SLOW_CLOSE_MS);
          est.escenaActual = ESCENA_NORMAL;
          est.lidProgress = p * SLEEP_LID_LEVEL; // 0 -> 0.95
          if (p >= 1.0f) { preState = PR_DONE; preStart = now; }
        } break;

        case PR_DONE:
          // queda "dormido": ojos en 95% cerrado
          est.escenaActual = ESCENA_DORMIDO;
          est.lidProgress = SLEEP_LID_LEVEL;
          break;
      }
    } break;

    case ESCENA_DESPERTAR: {
      switch (wakeState) {
        case WK_CLOSED_HOLD:
          est.escenaActual = ESCENA_DESPERTAR;
          est.lidProgress = 1.0f; // ojos cerrados
          if (now - wakeStart >= WAKE_CLOSED_HOLD_MS) {
            wakeState = WK_BLINK3;
            wakeStart = now;
          }
          break;

        case WK_BLINK3:
          est.escenaActual = ESCENA_NORMAL; // usamos forma normal para el blink
          if (blink_run_times(est, now, WAKE_BLINKS,
                              wakeBlinksDone, wakeBlinkRunning,
                              wakeBlinkPhaseStart, wakeBlinkPhase)) {
            // terminó los 3 parpadeos → deja abierto
            est.lidProgress = 0.0f;
            wakeState = WK_DONE;
          }
          break;

        case WK_DONE:
          // marca fin de despertar para disparar FELIZ a los 3s
          wakeEndedAt = now;
          escenas_set(ESCENA_NORMAL, est);
          break;
      }
    } break;



    case ESCENA_NORMAL: {
      // ¿Terminó de despertar hace 3s? -> ponerse feliz 3s
      if (wakeEndedAt != 0 && (now - wakeEndedAt >= WAKE_HAPPY_DELAY_MS)) {
        wakeEndedAt = 0;                 // evitar retriggers
        escenas_set(ESCENA_FELIZ, est);  // dispara emoción (dura 3s por EMOTION_DURATION_MS)
        break;                           // salir de NORMAL este ciclo
      }
  // Blink cada 3s (siempre)
      blink_tick(est, now, /*alwaysOn*/ true, BLINK_INTERVAL_MS);

      // Movimiento alterno cada 10s: derecha (2s) → HOLD 2s → vuelve rápido (0.4s) → siguiente izquierda…
      switch (moveState) {
        case MV_IDLE:
          if (now >= nextMoveAt) {
            moveState = MV_OUT; moveStart = now;
            est.leftEyeH = est.rightEyeH = EYE_H_NORMAL; // reset de alturas
          }
          break;

        case MV_OUT: { // ida en 2s
          float p = clamp01((float)(now - moveStart) / MOVE_OUT_MS);
          float e = easeInOutQuad(p);
          int amp = (int)(MOVE_AMPLITUDE_X * e);
          est.moveOffsetX = moveRightNext ? +amp : -amp;

          // achica el ojo del lado hacia donde mira
          if (moveRightNext) {
            est.rightEyeH = lerpInt(EYE_H_NORMAL, EYE_H_SHRINK, e);
            est.leftEyeH  = EYE_H_NORMAL;
          } else {
            est.leftEyeH  = lerpInt(EYE_H_NORMAL, EYE_H_SHRINK, e);
            est.rightEyeH = EYE_H_NORMAL;
          }

          if (p >= 1.0f) {
            moveState = MV_HOLD;           // ***nuevo: mantener 2s en el extremo***
            moveStart = now;
            // Aseguramos estar exactamente en el extremo
            est.moveOffsetX = moveRightNext ? +MOVE_AMPLITUDE_X : -MOVE_AMPLITUDE_X;
            // ojo del lado mirando queda achicado
            if (moveRightNext) est.rightEyeH = EYE_H_SHRINK; else est.leftEyeH = EYE_H_SHRINK;
          }
        } break;

        case MV_HOLD: // espera 2s en el extremo
          est.moveOffsetX = moveRightNext ? +MOVE_AMPLITUDE_X : -MOVE_AMPLITUDE_X;
          // mantener achique del ojo del lado mirando
          if (moveRightNext) {
            est.rightEyeH = EYE_H_SHRINK; est.leftEyeH = EYE_H_NORMAL;
          } else {
            est.leftEyeH  = EYE_H_SHRINK; est.rightEyeH = EYE_H_NORMAL;
          }
          if (now - moveStart >= MOVE_HOLD_MS) {
            moveState = MV_BACK; moveStart = now;
          }
          break;

        case MV_BACK: { // vuelve rápido
          float p = clamp01((float)(now - moveStart) / MOVE_BACK_MS);
          float e = easeInOutQuad(p);
          int amp = MOVE_AMPLITUDE_X - (int)(MOVE_AMPLITUDE_X * e);
          est.moveOffsetX = moveRightNext ? +amp : -amp;

          // volver alturas a normal
          if (moveRightNext) {
            est.rightEyeH = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);
          } else {
            est.leftEyeH  = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);
          }

          if (p >= 1.0f) {
            est.moveOffsetX = 0;
            est.leftEyeH = est.rightEyeH = EYE_H_NORMAL;
            moveState = MV_IDLE;
            moveRightNext = !moveRightNext;             // alterna lado
            nextMoveAt = now + NORMAL_MOVE_PERIOD_MS;   // programa próxima en 10s
          }
        } break;
      }

      est.escenaActual = ESCENA_NORMAL; // asegura forma “normal”
    } break;


    case ESCENA_ENOJADO:
    case ESCENA_FELIZ:
    case ESCENA_TRISTE:
    case ESCENA_RISA: {
      // Duran 3s y luego vuelven a NORMAL
      if (now - emotionStart >= EMOTION_DURATION_MS) {
        escenas_set(ESCENA_NORMAL, est);
      }
      // Nota: si querés blink también en emociones, descomenta:
      // blink_tick(est, now, true, BLINK_INTERVAL_MS);
    } break;

    case ESCENA_NINGUNA:
    default:
      // nada
      break;
  }
}
