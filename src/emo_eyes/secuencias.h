#pragma once
#include "estado.h"

// Inicialización del motor de escenas
void escenas_init(EstadoOjos& est);

// Cambiar de escena (resetea su estado interno)
void escenas_set(Escena nueva, EstadoOjos& est);

// Actualizar escena en curso
void escenas_update(EstadoOjos& est, unsigned long now);

// Escena actual
Escena escenas_actual();

// Zzz: se actualizan por tiempo y se dibujan encima del sprite
void zzz_update(unsigned long now, bool activo); // activo=true solo en DORMIDO
void zzz_render();                               // se llama desde render

