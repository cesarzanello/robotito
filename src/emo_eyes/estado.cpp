#include "estado.h"

void inicializarEstado(EstadoOjos& est) {
  est.escenaActual = ESCENA_NINGUNA;  // no dibujar nada por defecto
  est.lidProgress  = 0.0f;

  est.moveOffsetX  = 0;
  est.leftEyeH     = EYE_H_NORMAL;
  est.rightEyeH    = EYE_H_NORMAL;

  est.laughOffsetX = 0;
  est.laughOffsetY = 0;

  est.direccion    = DIR_NEUTRO;
}
