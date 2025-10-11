#pragma once
#include "estado.h"

// En este módulo van las “ejecuciones” o pasos de animación por escena,
// pero por pedido se deja SIN TIMERS y SIN ejecución automática.
// Cada función recibe el estado y parámetros (si los necesitás), y
// devuelve el estado actualizado. Llamalas manualmente desde loop()
// cuando quieras animar algo.

// Ejemplos de stubs (podés completarlos cuando quieras):
void tickParpadeo(EstadoOjos& est, float progreso01);       // 0→1 cierra, 1→0 abre
void tickMovimientoLateral(EstadoOjos& est, float progreso01, bool haciaDerecha);
void tickRisa(EstadoOjos& est, float fase01);
void tickDespertar(EstadoOjos& est, float etapa01);
void tickPreSueño(EstadoOjos& est, float etapa01);
