#include <Arduino.h>
#include "sensores.h"
#include "constantes.h"
#include <ESP32Servo.h>

void sensores_inicializar() {
  // Configura resolución y atenuación del ADC del ESP32
  analogReadResolution(LDR_RESOLUCION_BITS);
  analogSetAttenuation(ADC_11db); // ~0..3.3V
  // En ESP32 no hace falta pinMode para ADC, pero no molesta:
  pinMode(PIN_LDR_ADC, INPUT);
}

static int _ldr_leer_una_vez() {
  return analogRead(PIN_LDR_ADC); // 0..4095 (si 12 bits)
}

int ldr_leer_crudo() {
  long acc = 0;
  for (int i = 0; i < LDR_MUESTRAS_PROM; ++i) {
    acc += analogRead(PIN_LDR_ADC);
    delay(2); // ← pausa breve para dejar estabilizar el ADC
  }
  return (int)(acc / LDR_MUESTRAS_PROM);
}


int ldr_claridad_0a100(int lectura_cruda) {
  int maxv = (1 << LDR_RESOLUCION_BITS) - 1; // 4095 si 12 bits
  if (lectura_cruda < 0) lectura_cruda = 0;
  if (lectura_cruda > maxv) lectura_cruda = maxv;
  // Mapeo lineal 0..maxv -> 0..100
  int pct = (int)((lectura_cruda * 100L) / maxv);
  return pct;
}

bool ldr_es_claro(int lectura_cruda) {
  return lectura_cruda >= LDR_UMBRAL_CLARO;
}



static Servo g_servo;
static bool  g_attached = false;
static float g_currentAngle = SERVO_CENTER_DEG;

// pequeño filtro para suavizar (0..1)
static inline float slerp(float a, float b, float t){ return a + (b-a)*t; }

void servo_init() {
   Serial.println("[SERVO] init");
  // La lib necesita reservar timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  g_servo.setPeriodHertz(50); // servos a 50 Hz
  if (g_servo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US)) {
    g_attached = true;
    g_currentAngle = SERVO_CENTER_DEG;
    g_servo.write((int)g_currentAngle);
  }
}

void servo_update_from_offset(int moveOffsetX) {
  if (!g_attached) return;

  // mapear -MOVE_AMPLITUDE_X..+MOVE_AMPLITUDE_X -> SERVO_RIGHT_DEG..SERVO_LEFT_DEG
  // (cuando moveOffsetX es +, estás mirando a la derecha en tu lógica? ajusta si está invertido)
  long inMin  = -(long)MOVE_AMPLITUDE_X;
  long inMax  = +(long)MOVE_AMPLITUDE_X;
  long outMin = SERVO_RIGHT_DEG;
  long outMax = SERVO_LEFT_DEG;

  // map lineal estilo Arduino map(), pero protegido
  float norm = 0.0f;
  if (inMax != inMin) norm = (float)(moveOffsetX - inMin) / (float)(inMax - inMin);
  if (norm < 0) norm = 0; if (norm > 1) norm = 1;
  float target = outMin + norm * (outMax - outMin);

  // suavizado (subí/bajá 0.25 para cambiar “inercia”)
  g_currentAngle = slerp(g_currentAngle, target, 0.25f);
  g_servo.write((int)(g_currentAngle + 0.5f));
}

void servo_center() {
  if (!g_attached) return;
  g_currentAngle = slerp(g_currentAngle, (float)SERVO_CENTER_DEG, 0.15f);
  g_servo.write((int)(g_currentAngle + 0.5f));
}

