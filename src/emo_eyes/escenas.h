#pragma once
#include "estado.h"

// Dibuja la escena actual según ESTADO (no modifica timers ni estados)
void renderEscenaActual(EstadoOjos& est);

void overlay_borde_frio();
void overlay_borde_calor();
void overlay_sudor(unsigned long now);
