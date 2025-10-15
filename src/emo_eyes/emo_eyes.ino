#include <TFT_eSPI.h>
#include <SPI.h>
#include "constantes.h"
#include "estado.h"
#include "funciones.h"
#include "escenas.h"
#include "ejecuciones.h"
#include "sensores.h"
#include "secuencias.h"
#include <ESP32Servo.h>



TFT_eSPI tft = TFT_eSPI();
TFT_eSprite stage = TFT_eSprite(&tft);
EstadoOjos ESTADO;

void setup() {
  Serial.begin(115200);
  delay(200);  
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(BG_COLOR);
  
  
  
  stage.setColorDepth(8);
  stage.createSprite(STAGE_W, STAGE_H);

  inicializarEstado(ESTADO);
  sensores_inicializar();
  servo_init(); 
  escenas_init(ESTADO); // <- nombre correcto
  escenas_set(ESCENA_DESPERTAR, ESTADO);  // fuerza escena inicial visible
  renderEscenaActual(ESTADO);          // dibuja un frame ya mismo
  pinMode(PIN_TOUCH_ENOJO, INPUT_PULLUP);
  pinMode(PIN_TOUCH_RISA, INPUT_PULLUP);
  pinMode(PIN_TOUCH_FELIZ, INPUT_PULLUP);
  dht_init();




}


void loop() {
  unsigned long now = millis();

  // ====== TRACK DE ESCENA PARA DEBUG ======
  static Escena escenaPrev = ESCENA_NINGUNA;
  Escena escena = escenas_actual();
  
  bool enEmocion = (escena == ESCENA_ENOJADO || escena == ESCENA_FELIZ || escena == ESCENA_TRISTE || escena == ESCENA_RISA || escena == ESCENA_FRIO || escena == ESCENA_CALOR);

  if (escena != escenaPrev) {
    escenaPrev = escena;
    Serial.print("[SCENE] -> ");
    switch (escena) {
      case ESCENA_PRE_DORMIR: Serial.println("PRE_DORMIR"); break;
      case ESCENA_DESPERTAR:  Serial.println("DESPERTAR");  break;
      case ESCENA_NORMAL:     Serial.println("NORMAL");     break;
      case ESCENA_ENOJADO:    Serial.println("ENOJADO");    break;
      case ESCENA_FELIZ:      Serial.println("FELIZ");      break;
      case ESCENA_TRISTE:     Serial.println("TRISTE");     break;
      case ESCENA_RISA:       Serial.println("RISA");       break;
      default:                Serial.println("(OTRA)");     break;
    }
  }

// ===== TOUCH ENOJO (simple, con umbral fijo y debounce) =====
static bool touchEnojoEstable = true;            // HIGH = sin toque, LOW = tocado
static bool touchEnojoLeyendo = true;            // lectura cruda anterior
static unsigned long touchTsCambioEnojo = 0;
bool lecturaEnojo = digitalRead(PIN_TOUCH_ENOJO);

 if (lecturaEnojo != touchEnojoLeyendo) {
    touchEnojoLeyendo = lecturaEnojo;
    touchTsCambioEnojo = now;
  } else if ((now - touchTsCambioEnojo) >= TOUCH_DEBOUNCE_MS && lecturaEnojo != touchEnojoEstable) {
    // Cambio ESTABLE (debounced)
    touchEnojoEstable = lecturaEnojo;

    if (touchEnojoEstable == LOW) { // TOCADO → ENOJADO
      // no tocamos tu lógica: disparamos igual que con el botón
      Escena e = escenas_actual();
      if (e == ESCENA_NORMAL || e == ESCENA_FELIZ || e == ESCENA_TRISTE || e == ESCENA_RISA || e == ESCENA_FRIO || e == ESCENA_CALOR) {
        escenas_set(ESCENA_ENOJADO, ESTADO);
      }
      // si querés “mientras esté tocado está enojado”, re-llamá escenas_set(ESCENA_ENOJADO, ESTADO) cada loop
    } else { // SUELTO → volver a NORMAL si estabas en ENOJADO
      if (escenas_actual() == ESCENA_ENOJADO) {
        escenas_set(ESCENA_NORMAL, ESTADO);
      }
    }
  }
  bool touchEnojoPresionado = (touchEnojoEstable == HIGH);  



  // ===== TOUCH FELIZ (simple, con umbral fijo y debounce) =====
static bool touchFelizEstable = true;            // HIGH = sin toque, LOW = tocado
static bool touchFelizLeyendo = true;            // lectura cruda anterior
static unsigned long touchTsCambioFeliz = 0;
bool lecturaFeliz = digitalRead(PIN_TOUCH_FELIZ);

 if (lecturaFeliz != touchFelizLeyendo) {
    touchFelizLeyendo = lecturaFeliz;
    touchTsCambioFeliz = now;
  } else if ((now - touchTsCambioEnojo) >= TOUCH_DEBOUNCE_MS && lecturaFeliz != touchFelizEstable) {
    // Cambio ESTABLE (debounced)
    touchFelizEstable = lecturaFeliz;

    if (touchFelizEstable == LOW) { // TOCADO → ENOJADO
      // no tocamos tu lógica: disparamos igual que con el botón
      Escena e = escenas_actual();
      if (e == ESCENA_NORMAL || e == ESCENA_ENOJADO || e == ESCENA_TRISTE || e == ESCENA_RISA || e == ESCENA_FRIO || e == ESCENA_CALOR) {
        escenas_set(ESCENA_FELIZ, ESTADO);
      }
      // si querés “mientras esté tocado está enojado”, re-llamá escenas_set(ESCENA_ENOJADO, ESTADO) cada loop
    } else { // SUELTO → volver a NORMAL si estabas en ENOJADO
      if (escenas_actual() == ESCENA_FELIZ) {
        escenas_set(ESCENA_NORMAL, ESTADO);
      }
    }
  }
  bool touchFelizPresionado = (touchFelizEstable == HIGH);  

// ===== TOUCH RISA (simple, con umbral fijo y debounce) =====
static bool touchRisaEstable = true;            // HIGH = sin toque, LOW = tocado
static bool touchRisaLeyendo = true;            // lectura cruda anterior
static unsigned long touchTsCambioRisa = 0;
bool lecturaRisa = digitalRead(PIN_TOUCH_RISA);

 if (lecturaRisa != touchRisaLeyendo) {
    touchRisaLeyendo = lecturaRisa;
    touchTsCambioRisa = now;
  } else if ((now - touchTsCambioRisa) >= TOUCH_DEBOUNCE_MS && lecturaRisa != touchRisaEstable) {
    // Cambio ESTABLE (debounced)
    touchRisaEstable = lecturaRisa;

    if (touchRisaEstable == LOW) { // TOCADO → ENOJADO
      // no tocamos tu lógica: disparamos igual que con el botón
      Escena e = escenas_actual();
      if (e == ESCENA_NORMAL || e == ESCENA_FELIZ || e == ESCENA_TRISTE || e == ESCENA_ENOJADO || e == ESCENA_FRIO || e == ESCENA_CALOR) {
        escenas_set(ESCENA_RISA, ESTADO);
      }
      // si querés “mientras esté tocado está enojado”, re-llamá escenas_set(ESCENA_ENOJADO, ESTADO) cada loop
    } else { // SUELTO → volver a NORMAL si estabas en ENOJADO
      if (escenas_actual() == ESCENA_RISA) {
        escenas_set(ESCENA_NORMAL, ESTADO);
      }
    }
  }


// ----- Clima por DHT (no pisar emociones ni touch) -----
float tC, h;
bool ok = dht_leer(tC, h, now);

Escena e = escenas_actual();
enEmocion = (e == ESCENA_ENOJADO || e == ESCENA_FELIZ || e == ESCENA_TRISTE || e == ESCENA_RISA || e == ESCENA_FRIO || e == ESCENA_CALOR);

// sólo si hay lectura válida y NO hay emoción activa
if (ok && !enEmocion) {
  if (tC >= TEMP_CALOR_C) {
    if (e != ESCENA_CALOR) escenas_set(ESCENA_CALOR, ESTADO);
  } else if (tC <= TEMP_FRIO_C) {
    if (e != ESCENA_FRIO) escenas_set(ESCENA_FRIO, ESTADO);
  } else {
    // rango templado → si estabas en FRIO/CALOR, volver a NORMAL
    if (e == ESCENA_FRIO || e == ESCENA_CALOR) escenas_set(ESCENA_NORMAL, ESTADO);
  }
}


static unsigned long lastDhtMs = 0;
if (now - lastDhtMs >= 2000) {          // DHT11: no más rápido que 1 lectura / 2 s
  lastDhtMs = now;

  float tempC = NAN, hum = NAN;
  if (sensores_leer_tempHum(tempC, hum)) {
    Serial.print("Temp: "); Serial.print(tempC, 1); Serial.print(" °C  |  ");
    Serial.print("Hum: ");  Serial.print(hum,   0); Serial.println(" %");
  } else {
    Serial.println("Lectura inválida (NaN). Revisa cableado/tipo/tiempo.");
  }
}
// (opcional) log
// if (ok) { Serial.print("DHT T="); Serial.print(tC); Serial.print("C H="); Serial.println(h); }



  // ======= FIX #1: estado de presión correcto =========
    // <-- ANTES lo tenías invertido
  bool touchRisaPresionado = (touchRisaEstable == HIGH);      // <-- ANTES lo tenías invertido

  // ======= FIX #2 (recomendado): si cambió a ENOJADO en este frame, reflejalo =======
  escena = escenas_actual();
  enEmocion = (escena == ESCENA_ENOJADO || escena == ESCENA_FELIZ || escena == ESCENA_TRISTE || escena == ESCENA_RISA || escena == ESCENA_FRIO || escena == ESCENA_CALOR);

  // =========================================================
  // LUZ (LDR) con retardo de 3 s — NO pisa emociones NI touch
  // =========================================================
  static bool estadoClaro = ldr_es_claro(ldr_leer_crudo());
  static unsigned long tsCambioLuz = 0;

  // ======= FIX #3: solo correr LDR si NO hay toque y NO hay emoción =======
  if ((!touchEnojoPresionado || !touchRisaPresionado || !touchFelizPresionado) && !enEmocion) {
    int  crudoLdr   = ldr_leer_crudo();
    bool estaClaro  = ldr_es_claro(crudoLdr);

    if (estaClaro == estadoClaro) {
      tsCambioLuz = 0;
    } else {
      if (tsCambioLuz == 0) tsCambioLuz = now;
      bool aClaro   = (!estadoClaro && estaClaro);
      bool aOscuro  = ( estadoClaro && !estaClaro);
      unsigned long espera = aClaro ? LUZ_RETRASO_ENCENDER_MS : LUZ_RETRASO_APAGAR_MS;

      if ((now - tsCambioLuz) >= espera) {
        estadoClaro = estaClaro;
        tsCambioLuz = 0;
        if (aClaro)  escenas_set(ESCENA_DESPERTAR,  ESTADO);
        else         escenas_set(ESCENA_PRE_DORMIR, ESTADO);
      }
    }
  } else {
    tsCambioLuz = 0; // no acumular transición
  }

  // =========================================================
  // MOTOR + RENDER
  // =========================================================
  escenas_update(ESTADO, now);
  renderEscenaActual(ESTADO);

  delay(5);

  
}
