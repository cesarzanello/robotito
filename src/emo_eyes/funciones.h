#pragma once
#include "estado.h"

// Utilidades matemáticas
float clamp01(float v);
int   lerpInt(int a, int b, float t);
float easeInOutQuad(float x);

// Helpers de posicionamiento/dibujo base
void limpiarStage();
void empujarStageACentro();

// Dibujo de ojo “normal” (sin pupila)
void dibujarOjoNormal(int x, int y, int w, int h, float lid);

// Variantes de ojos (formas) — sin animación automática
void dibujarOjoEnojadoIzq (int x, int y, int w, int h);
void dibujarOjoEnojadoDer (int x, int y, int w, int h);

void dibujarOjoContentoIzq(int x, int y, int w, int h);
void dibujarOjoContentoDer(int x, int y, int w, int h);

void dibujarOjoTristeIzq  (int x, int y, int w, int h);
void dibujarOjoTristeDer  (int x, int y, int w, int h);

void dibujarOjoBostezoIzq (int x, int y, int w, int h);
void dibujarOjoBostezoDer (int x, int y, int w, int h);
