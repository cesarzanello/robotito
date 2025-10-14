#pragma once
#include <TFT_eSPI.h>

// ===== Colores =====
static const uint16_t BG_COLOR  = TFT_BLACK;
static const uint16_t EYE_FILL  = TFT_CYAN;

// ===== Geometría de los ojos =====
static const int EYE_W        = 60;
static const int EYE_H_NORMAL = 80;
static const int EYE_H_SHRINK = 70;
static const int RADIUS       = 15;
static const int GAP          = 15;

// ===== Canvas parcial =====
static const int MOVE_AMPLITUDE_X = 50;
static const int BASE_W  = (EYE_W * 2) + GAP;
static const int PAD     = 6;
static const int STAGE_W = BASE_W + (MOVE_AMPLITUDE_X * 2) + PAD * 2;
static const int STAGE_H = EYE_H_NORMAL + PAD * 2;

// ===== Lids sugeridos =====
static const float SLEEP_LID_LEVEL = 0.95f; // 95% cerrado
static const float HAPPY_LID       = 0.38f;
static const float SAD_LID         = 0.25f;

// ===== Blink genérico =====
static const unsigned long BLINK_INTERVAL_MS = 3000; // normal: cada 3s
static const unsigned long BLINK_CLOSE_MS    = 120;
static const unsigned long BLINK_HOLD_MS     = 60;
static const unsigned long BLINK_OPEN_MS     = 120;

// ===== Movimiento normal =====
static const unsigned long NORMAL_MOVE_PERIOD_MS = 10000; // cada 10s alterna lado
static const unsigned long MOVE_OUT_MS           = 400;  // ida lenta (2s)
static const unsigned long MOVE_HOLD_MS          = 2000;  // ***nuevo: espera 2s en el extremo***
static const unsigned long MOVE_BACK_MS          = 400;   // vuelve rápido


// ===== Secuencia DESPERTAR =====
static const unsigned long WAKE_CLOSED_HOLD_MS = 2000; // cerrados 2s
static const int WAKE_BLINKS = 3;
// Nuevo: tras despertar, a los 3s se pone contento
static const unsigned long WAKE_HAPPY_DELAY_MS = 3000;


// ===== Escenas de emoción (duración fija) =====
static const unsigned long EMOTION_DURATION_MS = 3000; // enojado/feliz/triste/risa

// ===== Secuencia PRE_DORMIR (fusionada) =====
static const unsigned long PRE_YAWN_MS     = 600;   // bostezo
static const unsigned long PRE_WAIT_MS     = 1000;  // esperas entre fases
static const unsigned long PRE_HALF_OPEN_LID = 0.50f * 1000; // 50% abierto (guardamos como 0.5 en código)
static const unsigned long PRE_SLOW_CLOSE_MS = 1500; // cierre muy lento

// ===== LDR (si usás sensor de luz; opcional) =====
static const int   PIN_LDR_ADC         = 32;   // ADC1
static const int   LDR_MUESTRAS_PROM   = 8;
static const int   LDR_RESOLUCION_BITS = 12;
static const int   LDR_UMBRAL_CLARO    = 2600;

// Esperas para confirmar cambio de luz
static const unsigned long LUZ_RETRASO_ENCENDER_MS = 3000; // luz se prende
static const unsigned long LUZ_RETRASO_APAGAR_MS   = 3000; // luz se apaga

// ===== Zzz (solo en DORMIDO) =====
static const int   ZZZ_MAX               = 6;      // cuántas a la vez
static const unsigned long ZZZ_SPAWN_MS  = 700;    // cada cuánto nace una
static const unsigned long ZZZ_LIFE_MS   = 2500;   // vida útil de cada Z
static const float ZZZ_SPEED_PX_S        = 28.0f;  // velocidad vertical (px/s, hacia arriba)
static const float ZZZ_DRIFT_PX_S        = 8.0f;   // deriva horizontal ± (px/s)
static const int   ZZZ_MIN_SIZE          = 1;      // setTextSize()
static const int   ZZZ_MAX_SIZE          = 2;      // setTextSize()

// --- Botón enojo ---
static const int  PIN_TOUCH_ENOJO     = 33;   // T8
static const unsigned long TOUCH_DEBOUNCE_MS = 30;

// --- Botón Risa---
static const int  PIN_TOUCH_RISA     = 35;   // T8
static const int  PIN_TOUCH_FELIZ     = 25;   // T8

// --- SERVO (ESP32 + ESP32Servo) ---
static const int  SERVO_PIN        = 14;   // pin señal del servo (PWM)
static const int  SERVO_MIN_US     = 500;  // pulso mínimo (us)
static const int  SERVO_MAX_US     = 2500; // pulso máximo (us)

// Ángulos de trabajo (ajustá si tu mecánica lo pide)
static const int  SERVO_RIGHT_DEG  = 45;   // ojo mira derecha -> ángulo mínimo
static const int  SERVO_CENTER_DEG = 90;   // centro
static const int  SERVO_LEFT_DEG   = 135;  // ojo mira izquierda -> ángulo máximo


