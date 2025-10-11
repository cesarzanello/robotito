#pragma once
#include <TFT_eSPI.h>
#include "constantes.h"

// Declaración de instancias gráficas (definidas en .ino)
extern TFT_eSPI tft;
extern TFT_eSprite stage;

// Escenas lógicas + modos de render que usamos en el motor
enum Escena {
  ESCENA_NINGUNA = 0,

  // Escenas lógicas
  ESCENA_PRE_DORMIR,   // (fusión pre-sueño + dormido, secuencia larga)
  ESCENA_DESPERTAR,    // secuencia de despertar
  ESCENA_NORMAL,       // normal con blink y movimientos
  ESCENA_ENOJADO,
  ESCENA_FELIZ,
  ESCENA_TRISTE,
  ESCENA_RISA,

  // Modos de render auxiliares que seguimos usando para dibujar
  ESCENA_DORMIDO,      // ojos 95% cerrados (render)
  ESCENA_PRE_SUENO,    // forma de bostezo (render)
  ESCENA_DESPERTAR_RENDER // frame de despertar (si lo querés distinguir)
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
};

void inicializarEstado(EstadoOjos& est);
