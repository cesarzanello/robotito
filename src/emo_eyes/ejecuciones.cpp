#include "ejecuciones.h"
#include "funciones.h"

// Cierra/abre los párpados según progreso
void tickParpadeo(EstadoOjos& est, float progreso01) {
  est.lidProgress = clamp01(progreso01);
}

// Desplaza lateral y achica el ojo del lado de la dirección (si querés)
void tickMovimientoLateral(EstadoOjos& est, float progreso01, bool haciaDerecha) {
  float e = easeInOutQuad(clamp01(progreso01));
  int offset = (int)(MOVE_AMPLITUDE_X * e);
  est.moveOffsetX = haciaDerecha ? offset : -offset;

  // Achique suave del ojo del lado hacia donde mira (opcional)
  int hFrom = EYE_H_NORMAL;
  int hTo   = EYE_H_SHRINK;
  if (haciaDerecha) {
    est.rightEyeH = lerpInt(hFrom, hTo, e);
    est.leftEyeH  = EYE_H_NORMAL;
  } else {
    est.leftEyeH  = lerpInt(hFrom, hTo, e);
    est.rightEyeH = EYE_H_NORMAL;
  }
}

// Micro offsets y lid para “risa” (si deseás)
void tickRisa(EstadoOjos& est, float fase01) {
  // Por ahora no hacer nada automáticamente.
  (void)fase01;
}

// Progresión de despertar (armá tu propio “blend” manual)
void tickDespertar(EstadoOjos& est, float etapa01) {
  // Sin timers: solo setea blend manual si querés.
  est.lidProgress = 1.0f - clamp01(etapa01); // 1→0
}

// Progresión de pre-sueño (bostezo, blinks, caída… a tu criterio)
void tickPreSueño(EstadoOjos& est, float etapa01) {
  // Stub sin acción
  (void)etapa01; // evitar warning si no se usa
}
