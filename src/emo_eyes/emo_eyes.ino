#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <math.h>

// ========================= Configuración del display ==========================
// Actualiza estas asignaciones de pines para que coincidan con tu cableado.
static constexpr int8_t TFT_SCK = 18;   // SPI clock
static constexpr int8_t TFT_MOSI = 23;  // SPI MOSI
static constexpr int8_t TFT_MISO = -1;  // Not used by most displays
static constexpr int8_t TFT_CS = 4;     // Chip select
static constexpr int8_t TFT_DC = 2;     // Data/command
static constexpr int8_t TFT_RST = 16;    // Reset pin (set to -1 if connected to ESP32 EN)
static constexpr int8_t TFT_BL = -1;    // Backlight control pin (set to -1 if tied to VCC)
// Objetos del bus SPI y del controlador de pantalla GC9A01A.
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotación */, true /* IPS */);
// ==============================================================================

// Geometría de los ojos y colores.
static constexpr uint16_t COLOR_FONDO = 0x0000; // Negro
static constexpr uint16_t COLOR_OJO = 0x07FF;   // Cian
static constexpr int16_t ANCHO_OJO = 60;
static constexpr int16_t ALTO_OJO = 80;
static constexpr int16_t ESPACIO_OJO = 10;

struct Ojo {
  int16_t centroX;
  int16_t centroY;
};

Ojo ojoIzquierdo;
Ojo ojoDerecho;

// Tiempos de la animación de parpadeo (en milisegundos).
static constexpr uint32_t INTERVALO_PARPADEO_MIN = 1800;
static constexpr uint32_t INTERVALO_PARPADEO_MAX = 3200;
static constexpr uint32_t DURACION_PARPADEO = 140; // Tiempo para cerrar o abrir los párpados

uint32_t proximoParpadeoEn = 0;
bool estaParpadeando = false;
bool ojosCerrandose = true;
uint32_t inicioFaseParpadeo = 0;
float progresoParpado = 0.0f; // 0 = abierto, 1 = completamente cerrado
float ultimoProgresoRenderizado = -1.0f;
bool ojosVisibles = false;

// Declaraciones anticipadas.
void programarSiguienteParpadeo();
void dibujarOjos(float cantidadParpado);
void dibujarOjo(const Ojo &ojo, float cantidadParpado);
void rellenarElipse(int16_t centroX, int16_t centroY, int16_t ancho, int16_t alto, uint16_t color);
float suavizadoEaseInOut(float t);

void setup() {
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  }

  gfx->begin();
  gfx->fillScreen(COLOR_FONDO);

  // Ubica los ojos aproximadamente centrados horizontalmente con un pequeño espacio.
  int16_t anchoPantalla = gfx->width();
  int16_t altoPantalla = gfx->height();
  int16_t desplazamientoOjoX = (ANCHO_OJO / 2) + ESPACIO_OJO;

  ojoIzquierdo.centroX = (anchoPantalla / 2) - desplazamientoOjoX;
  ojoDerecho.centroX = (anchoPantalla / 2) + desplazamientoOjoX;
  ojoIzquierdo.centroY = ojoDerecho.centroY = altoPantalla / 2;

  programarSiguienteParpadeo();
  dibujarOjos(0.0f);
}

void loop() {
  uint32_t now = millis();

  if (!estaParpadeando && now >= proximoParpadeoEn) {
    estaParpadeando = true;
    ojosCerrandose = true;
    inicioFaseParpadeo = now;
  }

  if (estaParpadeando) {
    uint32_t faseTranscurrida = now - inicioFaseParpadeo;
    float progresoFase = constrain(static_cast<float>(faseTranscurrida) / DURACION_PARPADEO, 0.0f, 1.0f);

    float progresoSuavizado = suavizadoEaseInOut(progresoFase);

    if (ojosCerrandose) {
      progresoParpado = progresoSuavizado;
      if (faseTranscurrida >= DURACION_PARPADEO) {
        // Cambia a la fase de apertura.
        ojosCerrandose = false;
        inicioFaseParpadeo = now;
      }
    } else {
      progresoParpado = 1.0f - progresoSuavizado;
      if (faseTranscurrida >= DURACION_PARPADEO) {
        estaParpadeando = false;
        progresoParpado = 0.0f;
        programarSiguienteParpadeo();
      }
    }
  }

  dibujarOjos(progresoParpado);
  delay(12); // Refresco aproximado de 80 FPS para una animación más fluida
}

void programarSiguienteParpadeo() {
  uint32_t intervalo = random(INTERVALO_PARPADEO_MIN, INTERVALO_PARPADEO_MAX);
  proximoParpadeoEn = millis() + intervalo;
}

void dibujarOjos(float cantidadParpado) {
  bool necesitaRedibujar = !ojosVisibles || estaParpadeando ||
                           fabsf(cantidadParpado - ultimoProgresoRenderizado) >= 0.01f;
  if (!necesitaRedibujar) {
    return;
  }

  ultimoProgresoRenderizado = cantidadParpado;
  ojosVisibles = true;

  gfx->startWrite();
  gfx->fillScreen(COLOR_FONDO);
  dibujarOjo(ojoIzquierdo, cantidadParpado);
  dibujarOjo(ojoDerecho, cantidadParpado);
  gfx->endWrite();
}

void dibujarOjo(const Ojo &ojo, float cantidadParpado) {
  int16_t mitadAncho = ANCHO_OJO / 2;
  int16_t mitadAlto = ALTO_OJO / 2;

  // Dibuja el ojo completo primero para asegurar que la reapertura quede limpia.
  rellenarElipse(ojo.centroX, ojo.centroY, ANCHO_OJO, ALTO_OJO, COLOR_OJO);

  if (cantidadParpado > 0.0f) {
    int16_t alturaCobertura = static_cast<int16_t>(ALTO_OJO * cantidadParpado * 0.5f);
    if (alturaCobertura > 0) {
      gfx->fillRect(ojo.centroX - mitadAncho, ojo.centroY - mitadAlto,
                    ANCHO_OJO, alturaCobertura, COLOR_FONDO);
      gfx->fillRect(ojo.centroX - mitadAncho, ojo.centroY + mitadAlto - alturaCobertura,
                    ANCHO_OJO, alturaCobertura, COLOR_FONDO);
    }
  }
}

void rellenarElipse(int16_t centroX, int16_t centroY, int16_t ancho, int16_t alto, uint16_t color) {
  float radioX = ancho / 2.0f;
  float radioY = alto / 2.0f;
  float radioXCuadrado = radioX * radioX;
  float radioYCuadrado = radioY * radioY;

  for (int16_t y = -static_cast<int16_t>(radioY); y <= static_cast<int16_t>(radioY); ++y) {
    float yNormalizado = static_cast<float>(y);
    float termino = 1.0f - (yNormalizado * yNormalizado) / radioYCuadrado;
    if (termino < 0.0f) {
      continue;
    }
    float anchoLinea = sqrtf(termino * radioXCuadrado);
    int16_t anchoLineaEntero = static_cast<int16_t>(anchoLinea + 0.5f);
    gfx->drawFastHLine(centroX - anchoLineaEntero, centroY + y, anchoLineaEntero * 2, color);
  }
}

float suavizadoEaseInOut(float t) {
  t = constrain(t, 0.0f, 1.0f);
  if (t < 0.5f) {
    return 2.0f * t * t;
  }
  return 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}
