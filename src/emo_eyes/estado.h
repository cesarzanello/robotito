#pragma once
#include <TFT_eSPI.h>
#include "constantes.h"

// Declaración de instancias gráficas (definidas en .ino)
extern TFT_eSPI tft;
extern TFT_eSprite stage;

// Escenas lógicas + modos de render que usamos en el motor
enum Escena {
  ESCENA_NINGUNA = 0,
  ESCENA_PRE_DORMIR,     // (única escena de dormir)
  ESCENA_DESPERTAR,
  ESCENA_NORMAL,
  ESCENA_ENOJADO,
  ESCENA_FELIZ,
  ESCENA_TRISTE,
  ESCENA_RISA,
  ESCENA_DESPERTAR_RENDER
};


// Direcciones de movimiento
enum Direccion {
  DIR_NEUTRO = 0,
  DIR_DERECHA,
  DIR_IZQUIERDA
};

// Estado principal
struct EstadoOjos {
  Escena escenaActual = ESCENA_NINGUNA;

  float lidProgress = 0.0f;

  int moveOffsetX = 0;
  int leftEyeH    = EYE_H_NORMAL;
  int rightEyeH   = EYE_H_NORMAL;

  int laughOffsetX = 0;
  int laughOffsetY = 0;

  Direccion direccion = DIR_NEUTRO;

  bool dormirBostezoFrame = false;
  bool dormirZzzActivo    = false;
};

void inicializarEstado(EstadoOjos& est);
