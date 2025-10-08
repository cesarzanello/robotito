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
static constexpr int8_t TFT_RST = 16;   // Reset pin (set to -1 if connected to ESP32 EN)
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

// Declaraciones anticipadas.
void dibujarOjos();
void dibujarOjo(const Ojo &ojo);
void rellenarElipse(int16_t centroX, int16_t centroY, int16_t ancho, int16_t alto, uint16_t color);

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

  dibujarOjos();
}

void loop() {
  // Los ojos permanecen fijos, sin animación.
  delay(1000);
}

void dibujarOjos() {
  gfx->startWrite();
  dibujarOjo(ojoIzquierdo);
  dibujarOjo(ojoDerecho);
  gfx->endWrite();
}

void dibujarOjo(const Ojo &ojo) {
  rellenarElipse(ojo.centroX, ojo.centroY, ANCHO_OJO, ALTO_OJO, COLOR_OJO);
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
