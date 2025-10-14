#include <Arduino.h>
#include "secuencias.h"    // incluye estado.h
#include "constantes.h"
#include "funciones.h"     // clamp01, easeInOutQuad, lerpInt
#include "escenas.h"       // render helpers (formas de ojos)
#include "ejecuciones.h"
#include "sensores.h"

// =====================================================
//                ESTADO INTERNO DEL MOTOR
// =====================================================
static Escena escenaActual = ESCENA_NINGUNA;

// ---------- Blink genérico ----------
static unsigned long blinkLastStart = 0;
static enum { B_IDLE, B_CLOSING, B_HOLD, B_OPENING } blinkState = B_IDLE;

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

// Parpadeos en bloque: corre N blinks y devuelve true al terminar
static bool blink_run_times(EstadoOjos& est, unsigned long now, int total,
                            int& done, bool& running, unsigned long& phaseStart, int& phase) {
  // phase: 0 close, 1 hold, 2 open
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
  return false;
}

// ---------- Movimiento NORMAL alterno ----------
static unsigned long nextMoveAt = 0;
static bool moveRightNext = true;
static enum { MV_IDLE, MV_OUT, MV_HOLD, MV_BACK } moveState;
static unsigned long moveStart = 0;

static void move_reset(unsigned long now) {
  moveState = MV_IDLE;
  moveStart = now;
  nextMoveAt = now + NORMAL_MOVE_PERIOD_MS; // primera a los 10s
  moveRightNext = true;
}

// ---------- Emociones (tiempo fijo) ----------
static unsigned long emotionStart = 0;

// =====================================================
//                 ESCENA: PRE_DORMIR (ÚNICA)
//   bostezo → 1s → 1 blink → 1s → 2 blinks → 0.5 abierto → 1s
//   → 3 blinks → 1s → cierre MUY lento → (cierra) → Zzz activas
// =====================================================
static enum {
  PR_IDLE,
  PR_YAWN, PR_WAIT1, PR_BLINK1, PR_WAIT2, PR_BLINK2, PR_HALF_OPEN, PR_WAIT3,
  PR_BLINK3, PR_WAIT4, PR_SLOW_CLOSE, PR_DONE
} preState;
static unsigned long preStart = 0;
static bool preBlinkRunning = false;
static int  preBlinkPhase = 0, preBlinksDone = 0;
static unsigned long preBlinkPhaseStart = 0;

static void pre_reset(unsigned long now) {
  preState = PR_YAWN; preStart = now;
  preBlinkRunning = false; preBlinkPhase = 0; preBlinksDone = 0; preBlinkPhaseStart = 0;
}

// =====================================================
//                 ESCENA: DESPERTAR
//    2s cerrados → 3 blinks → queda abierto → NORMAL
// =====================================================
static enum { WK_IDLE, WK_CLOSED_HOLD, WK_BLINK3, WK_DONE } wakeState;
static unsigned long wakeStart = 0;
static bool wakeBlinkRunning = false;
static int  wakeBlinkPhase = 0, wakeBlinksDone = 0;
static unsigned long wakeBlinkPhaseStart = 0;

// para disparar FELIZ 3s después de terminar DESPERTAR
static unsigned long wakeEndedAt = 0;

static void wake_reset(unsigned long now) {
  wakeState = WK_CLOSED_HOLD; wakeStart = now;
  wakeBlinkRunning = false; wakeBlinkPhase = 0; wakeBlinksDone = 0; wakeBlinkPhaseStart = 0;
}

// =====================================================
//                 ESCENA: NORMAL
// =====================================================
static unsigned long normalStart = 0;

static void normal_reset(unsigned long now) {
  normalStart = now;
  blink_reset();
  move_reset(now);
}

// =====================================================
//                         API
// =====================================================
void escenas_init(EstadoOjos& est) {
  escenaActual = ESCENA_NINGUNA;
  blink_reset();
  move_reset(millis());
  wakeEndedAt = 0;
  est.lidProgress = 0.0f;
  est.moveOffsetX = 0;
  est.leftEyeH = est.rightEyeH = EYE_H_NORMAL;
  est.dormirBostezoFrame = false;
  est.dormirZzzActivo    = false;
}

void escenas_set(Escena nueva, EstadoOjos& est) {
  unsigned long now = millis();
  escenaActual = nueva;

  // Si salimos de PRE_DORMIR, apagar flags/Zzz
  if (nueva != ESCENA_PRE_DORMIR) {
    est.dormirZzzActivo    = false;
    est.dormirBostezoFrame = false;
  }
  // Si cambiamos a cualquier escena que NO sea NORMAL,
  // no queremos disparar FELIZ post-despertar
  if (nueva != ESCENA_NORMAL) {
    wakeEndedAt = 0;
  }

  switch (nueva) {
    case ESCENA_PRE_DORMIR: {
      pre_reset(now);
      est.escenaActual       = ESCENA_PRE_DORMIR;
      est.dormirBostezoFrame = true;   // arranca con bostezo
      est.dormirZzzActivo    = false;  // Zzz se activan al final
    } break;

    case ESCENA_DESPERTAR:
      wake_reset(now);
      est.escenaActual = ESCENA_DESPERTAR;
      est.lidProgress  = 1.0f;  // cerrados
      break;

    case ESCENA_NORMAL:
      normal_reset(now);
      est.escenaActual = ESCENA_NORMAL;
      est.lidProgress  = 0.0f;
      est.moveOffsetX  = 0;
      est.leftEyeH = est.rightEyeH = EYE_H_NORMAL;
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

// =====================================================
//                        UPDATE
// =====================================================
void escenas_update(EstadoOjos& est, unsigned long now) {
  switch (escenaActual) {

    // ----------------- DORMIR (única) -----------------
    case ESCENA_PRE_DORMIR: {
      switch (preState) {
        case PR_YAWN:
          est.escenaActual       = ESCENA_PRE_DORMIR;
          est.dormirBostezoFrame = true;  // dibuja bostezo
          if (now - preStart >= PRE_YAWN_MS) { preState = PR_WAIT1; preStart = now; }
          break;

        case PR_WAIT1:
          est.dormirBostezoFrame = false;
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_BLINK1; preStart = now; }
          break;

        case PR_BLINK1:
          est.dormirBostezoFrame = false;
          if (blink_run_times(est, now, 1,
                              preBlinksDone, preBlinkRunning,
                              preBlinkPhaseStart, preBlinkPhase)) {
            preBlinksDone = 0; preBlinkRunning = false;
            preState = PR_WAIT2; preStart = now;
          }
          est.escenaActual = ESCENA_PRE_DORMIR;
          break;

        case PR_WAIT2:
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_BLINK2; preStart = now; }
          break;

        case PR_BLINK2:
          if (blink_run_times(est, now, 2,
                              preBlinksDone, preBlinkRunning,
                              preBlinkPhaseStart, preBlinkPhase)) {
            preBlinksDone = 0; preBlinkRunning = false;
            preState = PR_HALF_OPEN; preStart = now;
          }
          est.escenaActual = ESCENA_PRE_DORMIR;
          break;

        case PR_HALF_OPEN:
          est.escenaActual = ESCENA_PRE_DORMIR;
          est.lidProgress  = 0.5f; // abre hasta la mitad
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_WAIT3; preStart = now; }
          break;

        case PR_WAIT3:
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_BLINK3; preStart = now; }
          break;

        case PR_BLINK3:
          if (blink_run_times(est, now, 3,
                              preBlinksDone, preBlinkRunning,
                              preBlinkPhaseStart, preBlinkPhase)) {
            preBlinksDone = 0; preBlinkRunning = false;
            preState = PR_WAIT4; preStart = now;
          }
          est.escenaActual = ESCENA_PRE_DORMIR;
          break;

        case PR_WAIT4:
          if (now - preStart >= PRE_WAIT_MS) { preState = PR_SLOW_CLOSE; preStart = now; }
          break;

        case PR_SLOW_CLOSE: {
          // cierre muy despacio 0 -> 0.95
          float p = clamp01((float)(now - preStart) / PRE_SLOW_CLOSE_MS);
          est.escenaActual       = ESCENA_PRE_DORMIR;
          est.dormirBostezoFrame = false;
          est.lidProgress        = p * SLEEP_LID_LEVEL;
          if (p >= 1.0f) { preState = PR_DONE; preStart = now; }
        } break;

        case PR_DONE:
          // Cerrado (≈95%) y Zzz activas
          est.escenaActual       = ESCENA_PRE_DORMIR;
          est.dormirBostezoFrame = false;
          est.lidProgress        = SLEEP_LID_LEVEL;
          est.dormirZzzActivo    = true; // SOLO ahora hay Zzz
          break;
      }

      // Actualizar Zzz (solo spawnea si dormirZzzActivo=true)
      zzz_update(now, est.dormirZzzActivo);
    } break;

    // ----------------- DESPERTAR -----------------
    case ESCENA_DESPERTAR: {
      switch (wakeState) {
        case WK_CLOSED_HOLD:
          est.escenaActual = ESCENA_DESPERTAR;
          est.lidProgress  = 1.0f;
          if (now - wakeStart >= WAKE_CLOSED_HOLD_MS) {
            wakeState = WK_BLINK3; wakeStart = now;
          }
          break;

        case WK_BLINK3:
          est.escenaActual = ESCENA_NORMAL; // forma normal para parpadeo
          if (blink_run_times(est, now, WAKE_BLINKS,
                              wakeBlinksDone, wakeBlinkRunning,
                              wakeBlinkPhaseStart, wakeBlinkPhase)) {
            est.lidProgress = 0.0f; // abierto
            wakeState = WK_DONE;
          }
          break;

        case WK_DONE:
          wakeEndedAt = now;              // marcar fin de despertar
          escenas_set(ESCENA_NORMAL, est);
          break;
      }
      // Zzz no corren fuera de dormir
      // zzz_update(now, false); // opcional si querés que las que queden "se vayan"
    } break;

    // ----------------- NORMAL -----------------
    case ESCENA_NORMAL: {
      // ¿3s después de despertar? → FELIZ (3s)
      if (wakeEndedAt != 0 && (now - wakeEndedAt >= WAKE_HAPPY_DELAY_MS)) {
        wakeEndedAt = 0;
        escenas_set(ESCENA_FELIZ, est);
        break;
      }

      // Blink cada 3s, siempre
      blink_tick(est, now, true, BLINK_INTERVAL_MS);

      // Movimiento alterno cada 10s:
      // ida 2s → HOLD 2s → vuelta 0.4s → programa próxima
      switch (moveState) {
        case MV_IDLE:
          if (now >= nextMoveAt) {
            moveState = MV_OUT; moveStart = now;
            est.leftEyeH = est.rightEyeH = EYE_H_NORMAL;
          }
          break;

        case MV_OUT: {
          float p = clamp01((float)(now - moveStart) / MOVE_OUT_MS);
          float e = easeInOutQuad(p);
          int amp = (int)(MOVE_AMPLITUDE_X * e);
          est.moveOffsetX = moveRightNext ? +amp : -amp;

          if (moveRightNext) {
            est.rightEyeH = lerpInt(EYE_H_NORMAL, EYE_H_SHRINK, e);
            est.leftEyeH  = EYE_H_NORMAL;
          } else {
            est.leftEyeH  = lerpInt(EYE_H_NORMAL, EYE_H_SHRINK, e);
            est.rightEyeH = EYE_H_NORMAL;
          }

          if (p >= 1.0f) {
            moveState = MV_HOLD; moveStart = now;
            est.moveOffsetX = moveRightNext ? +MOVE_AMPLITUDE_X : -MOVE_AMPLITUDE_X;
            if (moveRightNext) est.rightEyeH = EYE_H_SHRINK; else est.leftEyeH = EYE_H_SHRINK;
          }
        } break;

        case MV_HOLD:
          est.moveOffsetX = moveRightNext ? +MOVE_AMPLITUDE_X : -MOVE_AMPLITUDE_X;
          if (moveRightNext) { est.rightEyeH = EYE_H_SHRINK; est.leftEyeH  = EYE_H_NORMAL; }
          else               { est.leftEyeH  = EYE_H_SHRINK; est.rightEyeH = EYE_H_NORMAL; }
          if (now - moveStart >= MOVE_HOLD_MS) {
            moveState = MV_BACK; moveStart = now;
          }
          break;

        case MV_BACK: {
          float p = clamp01((float)(now - moveStart) / MOVE_BACK_MS);
          float e = easeInOutQuad(p);
          int amp = MOVE_AMPLITUDE_X - (int)(MOVE_AMPLITUDE_X * e);
          est.moveOffsetX = moveRightNext ? +amp : -amp;

          if (moveRightNext) est.rightEyeH = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);
          else               est.leftEyeH  = lerpInt(EYE_H_SHRINK, EYE_H_NORMAL, e);

          if (p >= 1.0f) {
            est.moveOffsetX = 0;
            est.leftEyeH = est.rightEyeH = EYE_H_NORMAL;
            moveState = MV_IDLE;
            moveRightNext = !moveRightNext;
            nextMoveAt = now + NORMAL_MOVE_PERIOD_MS;
          }
        } break;
      }

      est.escenaActual = ESCENA_NORMAL;
      // zzz_update(now, false); // fuera de dormir no spawneamos
    } break;

    // ----------------- EMOCIONES (3s) -----------------
    // ----------------- EMOCIONES (3s) -----------------
    case ESCENA_ENOJADO:
    case ESCENA_FELIZ:
    case ESCENA_TRISTE: {
      if (now - emotionStart >= EMOTION_DURATION_MS) {
        escenas_set(ESCENA_NORMAL, est);
      }
      // Si querés blink en estas, podés activar:
      // blink_tick(est, now, true, BLINK_INTERVAL_MS);
    } break;

    case ESCENA_RISA: {
      unsigned long t = now - emotionStart;

      // 1) Fin a los 3 s → volver a normal y limpiar offsets
      if (t >= EMOTION_DURATION_MS) {
        est.lidProgress = 0.0f;                   // ojos abiertos
        est.moveOffsetX = 0;                      // sin shake
        est.leftEyeH = est.rightEyeH = EYE_H_NORMAL;
        escenas_set(ESCENA_NORMAL, est);
        break;
      }

      // 2) Micro-parpadeo “risa”
      //    ciclo ~125ms; lidProgress oscila 0..0.6 (semi-cerrado rápido)
      float fase = (t % 125UL) / 125.0f;
      float sBlink = (sinf(fase * 2.0f * 3.1415926f) * 0.5f + 0.5f);  // 0..1
      est.lidProgress = 0.6f * sBlink;

      // 3) Shake horizontal + leve vertical “natural”
      //    amplitud pequeña: ±5 px X, ±3 px Y simulados con altura de ojos
      float sx = sinf(t / 80.0f);    // más rápido en X
      float sy = sinf(t / 110.0f);   // más lento en “Y”

      est.moveOffsetX = (int)(5.0f * sx);   // usa el offset X existente del render

      // simulamos una mini vibración vertical achicando/agrandando un ojo sutilmente
      // (si no te gusta, podés borrar estas dos líneas)
      est.leftEyeH  = EYE_H_NORMAL - (int)(3.0f * sy);
      est.rightEyeH = EYE_H_NORMAL + (int)(3.0f * sy);

      // NOTA: No llamamos a blink_tick aquí para que no pise el micro-parpadeo.
      //       El render usa la forma “normal” para RISA, pero con estos offsets.
    } break;

  }
  if (escenaActual == ESCENA_NORMAL || escenaActual == ESCENA_RISA ) {
    servo_update_from_offset(est.moveOffsetX);
  } else {
    servo_center();
  }

}

// =====================================================
//                       Z z z
//   (solo activas cuando termina de cerrar en PRE_DORMIR)
// =====================================================
struct ZPart {
  bool active;
  float x, y;            // posición en el sprite
  float vx, vy;          // px/s
  unsigned long born;    // ms
  unsigned long life;    // ms
  uint8_t size;          // 1..3
};
static ZPart Z[ZZZ_MAX];
static unsigned long zNextSpawn = 0;
static unsigned long zLastUpdate = 0;

// “centro de boca” aprox en el sprite
static inline int z_spawn_x() { return STAGE_W / 2; }
static inline int z_spawn_y() { return STAGE_H - 8; }

static void z_spawn(unsigned long now) {
  for (int i = 0; i < ZZZ_MAX; ++i) if (!Z[i].active) {
    Z[i].active = true;
    Z[i].x = (float)z_spawn_x() + (float)(random(-6, 7));
    Z[i].y = (float)z_spawn_y();
    float dir = (random(0,2) ? 1.0f : -1.0f);
    Z[i].vx = dir * ZZZ_DRIFT_PX_S * (0.6f + (random(0,41)/100.0f));
    Z[i].vy = -ZZZ_SPEED_PX_S * (0.8f + (random(0,41)/100.0f));
    Z[i].born = now;
    Z[i].life = ZZZ_LIFE_MS;
    Z[i].size = (uint8_t)random(ZZZ_MIN_SIZE, ZZZ_MAX_SIZE + 1);
    return;
  }
}

void zzz_update(unsigned long now, bool activo) {
  if (zLastUpdate == 0) zLastUpdate = now;
  float dt = (now - zLastUpdate) / 1000.0f;
  zLastUpdate = now;

  if (activo) {
    if (now >= zNextSpawn) {
      z_spawn(now);
      zNextSpawn = now + ZZZ_SPAWN_MS;
    }
  } else {
    zNextSpawn = now + ZZZ_SPAWN_MS;
  }

  for (int i = 0; i < ZZZ_MAX; ++i) if (Z[i].active) {
    if (now - Z[i].born >= Z[i].life) { Z[i].active = false; continue; }
    Z[i].x += Z[i].vx * dt;
    Z[i].y += Z[i].vy * dt;
    if (Z[i].y < -10 || Z[i].x < -10 || Z[i].x > STAGE_W + 10) {
      Z[i].active = false;
    }
  }
}

void zzz_render() {
  stage.setTextColor(TFT_DARKGREY, BG_COLOR);
  stage.setTextDatum(TL_DATUM);
  for (int i = 0; i < ZZZ_MAX; ++i) if (Z[i].active) {
    stage.setTextSize(Z[i].size);
    stage.drawChar('Z', (int)Z[i].x, (int)Z[i].y);
  }
}
