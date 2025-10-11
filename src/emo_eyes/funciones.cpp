#include "funciones.h"

// ==== Utilidades matemáticas ====
float clamp01(float v){ return v < 0 ? 0 : (v > 1 ? 1 : v); }

int lerpInt(int a, int b, float t) {
  t = clamp01(t);
  return a + (int)((b - a) * t);
}

float easeInOutQuad(float x) {
  x = clamp01(x);
  return (x < 0.5f) ? (2.0f * x * x)
                    : (1.0f - ((-2.0f * x + 2.0f) * (-2.0f * x + 2.0f)) / 2.0f);
}

// ==== Helpers de stage ====
void limpiarStage() {
  stage.fillSprite(BG_COLOR);
}

void empujarStageACentro() {
  int screenX = (tft.width()  - STAGE_W) / 2;
  int screenY = (tft.height() - STAGE_H) / 2;
  stage.pushSprite(screenX, screenY);
}

// ==== Dibujos base ====
void dibujarOjoNormal(int x, int y, int w, int h, float lid) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);

  int halfH = h / 2;
  int lidH  = (int)(clamp01(lid) * halfH);
  if (lidH > 0) {
    stage.fillRect(x, y, w, lidH, BG_COLOR);             // tapa superior
    stage.fillRect(x, y + h - lidH, w, lidH, BG_COLOR);  // tapa inferior
  }
}

// ENOJADO
void dibujarOjoEnojadoIzq(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  stage.fillTriangle(x, y, x + w, y, x + w, y + tilt, BG_COLOR);
}

void dibujarOjoEnojadoDer(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  stage.fillTriangle(x, y + tilt, x, y, x + w, y, BG_COLOR);
}

// CONTENTO
void dibujarOjoContentoIzq(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int upperInner = (int)(h * 0.14f);
  stage.fillTriangle(x + w, y,  x, y,  x, y + upperInner, BG_COLOR);
  int liftOuter = (int)(h * 0.40f);
  int liftInner = (int)(h * 0.22f);
  stage.fillTriangle(x,     y + h, x + w, y + h, x + w, y + h - liftInner, BG_COLOR);
  stage.fillTriangle(x,     y + h, x,     y + h - liftOuter, x + w, y + h - liftInner, BG_COLOR);
}

void dibujarOjoContentoDer(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int upperInner = (int)(h * 0.14f);
  stage.fillTriangle(x, y + upperInner,  x, y,  x + w, y, BG_COLOR);
  int liftOuter = (int)(h * 0.40f);
  int liftInner = (int)(h * 0.22f);
  stage.fillTriangle(x,     y + h, x + w, y + h, x,     y + h - liftInner, BG_COLOR);
  stage.fillTriangle(x + w, y + h, x + w, y + h - liftOuter, x, y + h - liftInner, BG_COLOR);
}

// TRISTE
void dibujarOjoTristeIzq(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int upperOuter = (int)(h * 0.16f);
  stage.fillTriangle(x, y + upperOuter,  x, y,  x + w, y, BG_COLOR);
  int dropOuter = (int)(h * 0.24f);
  int dropInner = (int)(h * 0.10f);
  stage.fillTriangle(x,     y + h, x + w, y + h, x,     y + h - dropOuter, BG_COLOR);
  stage.fillTriangle(x + w, y + h, x + w, y + h - dropInner, x, y + h - dropOuter, BG_COLOR);
}

void dibujarOjoTristeDer(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int upperOuter = (int)(h * 0.16f);
  stage.fillTriangle(x + w, y + upperOuter,  x + w, y,  x, y, BG_COLOR);
  int dropOuter = (int)(h * 0.24f);
  int dropInner = (int)(h * 0.10f);
  stage.fillTriangle(x + w, y + h, x, y + h, x + w, y + h - dropOuter, BG_COLOR);
  stage.fillTriangle(x,     y + h, x, y + h - dropInner, x + w, y + h - dropOuter, BG_COLOR);
}

// BOSTEZO (párpado inferior diagonal hacia el centro)
void dibujarOjoBostezoIzq(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  stage.fillTriangle(x, y + h, x + w, y + h, x + w, y + h - tilt, BG_COLOR);
}

void dibujarOjoBostezoDer(int x, int y, int w, int h) {
  stage.fillRoundRect(x, y, w, h, RADIUS, EYE_FILL);
  int tilt = (int)(h * 0.35f);
  stage.fillTriangle(x, y + h - tilt, x, y + h, x + w, y + h, BG_COLOR);
}
