#include <Arduino.h>
#include "sensores.h"
#include "constantes.h"

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
