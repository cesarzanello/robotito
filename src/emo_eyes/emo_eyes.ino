#include <TFT_eSPI.h>
#include <SPI.h>

#include "constantes.h"
#include "estado.h"
#include "funciones.h"
#include "escenas.h"
#include "ejecuciones.h"
#include "sensores.h"
#include "secuencias.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite stage = TFT_eSprite(&tft);
EstadoOjos ESTADO;

void setup() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(BG_COLOR);

  stage.setColorDepth(8);
  stage.createSprite(STAGE_W, STAGE_H);

  inicializarEstado(ESTADO);
  sensores_inicializar();

  escenas_init(ESTADO); // <- nombre correcto
  escenas_set(ESCENA_DESPERTAR, ESTADO);  // fuerza escena inicial visible
  renderEscenaActual(ESTADO);          // dibuja un frame ya mismo

  // Arranque, si querés:
  // escenas_set(ESCENA_PRE_DORMIR, ESTADO);
}


void loop() {
  unsigned long now = millis();

  // --- Lectura de luz ---
  int  crudo     = ldr_leer_crudo();
  bool estaClaro = ldr_es_claro(crudo);

  // --- Detección estable (3s) de cambios de luz ---
  // estado actual memorizado de "claro/oscuro"
  static bool estadoClaro = estaClaro;      // arranca con la primera lectura
  // timestamp desde que empezó a cambiar
  static unsigned long tsCambio = 0;

  // Si el nivel coincide con el estado memorizado, no hay cambio → limpiar temporizador
  if (estaClaro == estadoClaro) {
    tsCambio = 0; // sin transición en curso
  } else {
    // Hay una transición en curso (oscuro->claro o claro->oscuro)
    if (tsCambio == 0) {
      tsCambio = now; // iniciar conteo de estabilidad
    } else {
      // ¿se sostuvo suficiente tiempo?
      bool flancoOscuroAClaro = (!estadoClaro && estaClaro);
      bool flancoClaroAOscuro = ( estadoClaro && !estaClaro);

      if (flancoOscuroAClaro && (now - tsCambio >= LUZ_RETRASO_ENCENDER_MS)) {
        // Confirmado: se prendió la luz por 3s
        estadoClaro = estaClaro; // consolidar estado
        tsCambio = 0;

        // Si no estamos ya despertando, lanzamos DESPERTAR
        if (escenas_actual() != ESCENA_DESPERTAR) {
          escenas_set(ESCENA_DESPERTAR, ESTADO);
        }
      }
      else if (flancoClaroAOscuro && (now - tsCambio >= LUZ_RETRASO_APAGAR_MS)) {
        // Confirmado: se apagó la luz por 3s
        estadoClaro = estaClaro; // consolidar estado
        tsCambio = 0;

        // Si no estamos ya en la secuencia pre_dormir, lanzamos PRE_DORMIR
        if (escenas_actual() != ESCENA_PRE_DORMIR) {
          escenas_set(ESCENA_PRE_DORMIR, ESTADO);
        }
      }
    }
  }

  // --- Actualizar escena en curso (maneja tiempos internos, blinks, movimiento, etc.) ---
  escenas_update(ESTADO, now);

  // --- Dibujar ---
  renderEscenaActual(ESTADO);

  delay(5);
}





