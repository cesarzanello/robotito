#include <Arduino.h>
#include "sensores.h"
#include "constantes.h"
#include <ESP32Servo.h>
#include <DHT.h>

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


// Servo global, igual que en tu sketch que funciona
static Servo g_servo;
static bool  g_attached = false;
static float g_currentAngle = SERVO_CENTER_DEG;

static inline float slerp(float a, float b, float t) { return a + (b - a) * t; }

void servo_init() {
  Serial.println("[SERVO] init");

  // Igual que tu sketch: reservar timers y configurar el periodo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  g_servo.setPeriodHertz(50);

  if (g_servo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US)) {
    g_attached = true;
    g_currentAngle = SERVO_CENTER_DEG;
    g_servo.write((int)g_currentAngle);
    Serial.println("[SERVO] attach OK");
  } else {
    Serial.println("[SERVO] attach FAIL");
  }
}

// Mapea -MOVE_AMPLITUDE_X..+MOVE_AMPLITUDE_X a RIGHT..LEFT y suaviza un poco
void servo_update_from_offset(int moveOffsetX) {
//   if (!g_attached) return;
  const long inMin  = -(long)MOVE_AMPLITUDE_X;
  const long inMax  = +(long)MOVE_AMPLITUDE_X;
  const long outMin = SERVO_RIGHT_DEG;   // invertí con outMax si va al revés
  const long outMax = SERVO_LEFT_DEG;

  float norm = (inMax != inMin) ? (float)(moveOffsetX - inMin) / (float)(inMax - inMin) : 0.5f;
  if (norm < 0) norm = 0; if (norm > 1) norm = 1;

  float target = outMin + norm * (outMax - outMin);
  g_currentAngle = slerp(g_currentAngle, target, 0.25f);  // suavizado simple
  g_servo.write((int)(g_currentAngle + 0.5f));
}

void servo_center() {
  if (!g_attached) return;
  g_currentAngle = slerp(g_currentAngle, (float)SERVO_CENTER_DEG, 0.18f);
  g_servo.write((int)(g_currentAngle + 0.5f));
}


//DHT
static DHT g_dht(DHT_PIN, DHT11);
static unsigned long g_dhtLast = 0;
static float g_lastT = NAN, g_lastH = NAN;
static bool  g_lastOk = false;

void dht_init() {
  g_dht.begin();
  g_dhtLast = 0;
  g_lastOk  = false;
}

// Lee como máx. cada DHT_PERIOD_MS. Devuelve true si hay lectura válida.
bool dht_leer(float& tempC, float& hum, unsigned long now) {
  if (now - g_dhtLast < DHT_PERIOD_MS && g_lastOk) {
    tempC = g_lastT; hum = g_lastH; return true;
  }
  g_dhtLast = now;

  float h = g_dht.readHumidity();
  float t = g_dht.readTemperature(); // °C

  if (!isnan(h) && !isnan(t)) {
    g_lastOk = true; g_lastT = t; g_lastH = h;
    tempC = t; hum = h; return true;
  }
  g_lastOk = false;
  tempC = NAN; hum = NAN; return false;
}

bool sensores_leer_tempHum(float &tempC, float &hum) {
  hum   = g_dht.readHumidity();
  tempC = g_dht.readTemperature(); // °C

  if (isnan(hum) || isnan(tempC)) {
    return false;
  }
  return true;
}
