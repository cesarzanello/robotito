#pragma once
#include "estado.h"

// Inicializa ADC/atenuación, etc.
void sensores_inicializar();

// Lee el LDR con promedio y retorna 0..4095 (según resolución configurada)
int  ldr_leer_crudo();

// Convierte lectura a 0..100 (% de “claridad”; 0 = oscuro, 100 = muy claro)
int  ldr_claridad_0a100(int lectura_cruda);

// “Digital” por software: true si está por encima del umbral de claridad
bool ldr_es_claro(int lectura_cruda);

// DHT11
void dht_init();
bool dht_leer(float& tempC, float& hum, unsigned long now);
bool sensores_leer_tempHum(float &tempC, float &hum) ;


void servo_init();                       // llamalo en setup()
void servo_update_from_offset(int x);    // seguir ESTADO.moveOffsetX
void servo_center();  