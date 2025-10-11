#include "escenas.h"
#include "funciones.h"
#include "secuencias.h"


void renderEscenaActual(EstadoOjos& est) {
  // Si no hay nada para dibujar, salgo
  if (est.escenaActual == ESCENA_NINGUNA) return;

  limpiarStage();

  int centerInStage = STAGE_W / 2;
  int baseLeftX  = centerInStage - (BASE_W / 2) + est.moveOffsetX + est.laughOffsetX;
  int baseRightX = baseLeftX + EYE_W + GAP;
  int leftY  = (STAGE_H - est.leftEyeH)  / 2 + est.laughOffsetY;
  int rightY = (STAGE_H - est.rightEyeH) / 2 + est.laughOffsetY;

  switch (est.escenaActual) {

    case ESCENA_NORMAL:
      dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
      dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      break;

    case ESCENA_ENOJADO:
      dibujarOjoEnojadoIzq(baseLeftX,  leftY,  EYE_W, est.leftEyeH);
      dibujarOjoEnojadoDer(baseRightX, rightY, EYE_W, est.rightEyeH);
      break;

    case ESCENA_FELIZ:
      dibujarOjoContentoIzq(baseLeftX,  leftY,  EYE_W, est.leftEyeH);
      dibujarOjoContentoDer(baseRightX, rightY, EYE_W, est.rightEyeH);
      break;

    case ESCENA_TRISTE:
      dibujarOjoTristeIzq(baseLeftX,  leftY,  EYE_W, est.leftEyeH);
      dibujarOjoTristeDer(baseRightX, rightY, EYE_W, est.rightEyeH);
      break;

    case ESCENA_RISA:
      // forma base; el “tembleque” si lo querés va en el update
      dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
      dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      break;

    case ESCENA_DESPERTAR:
    case ESCENA_DESPERTAR_RENDER:
      // si usás un frame de despertar, podés dibujar igual que normal
      dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
      dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      break;
    case ESCENA_PRE_DORMIR: {
      if (est.dormirBostezoFrame) {
        // frame de bostezo
        dibujarOjoBostezoIzq(baseLeftX,  leftY,  EYE_W, est.leftEyeH);
        dibujarOjoBostezoDer(baseRightX, rightY, EYE_W, est.rightEyeH);
      } else {
        // resto de fases (parpadeos, 1/2 abierto, cierre lento, cerrado final)
        dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
        dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      }

      // Zzz sólo cuando terminó de cerrar (flag seteado al final de la secuencia)
      if (est.dormirZzzActivo) {
        zzz_render();
      }
    } break;


    case ESCENA_NINGUNA:
    default:
      // nada
      break;
  }

  empujarStageACentro();
}

