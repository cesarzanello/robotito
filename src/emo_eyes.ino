#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <math.h>

// ========================= Configuración del display ==========================
// Actualiza estas asignaciones de pines para que coincidan con tu cableado.
static constexpr int8_t TFT_SCK = 18;   // Reloj SPI
static constexpr int8_t TFT_MOSI = 23;  // MOSI SPI
static constexpr int8_t TFT_MISO = -1;  // No se usa en la mayoría de las pantallas
static constexpr int8_t TFT_CS = 5;     // Selección de chip
static constexpr int8_t TFT_DC = 2;     // Datos/comando
static constexpr int8_t TFT_RST = 4;    // Pin de reinicio (usa -1 si está conectado al EN del ESP32)
static constexpr int8_t TFT_BL = 15;    // Control de retroiluminación (usa -1 si está directo a VCC)

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
static constexpr uint32_t INTERVALO_PARPADEO_MIN = 2500;
static constexpr uint32_t INTERVALO_PARPADEO_MAX = 4500;
static constexpr uint32_t DURACION_PARPADEO = 200; // Tiempo para cerrar o abrir los párpados

uint32_t proximoParpadeoEn = 0;
bool estaParpadeando = false;
bool ojosCerrandose = true;
uint32_t inicioFaseParpadeo = 0;
float progresoParpado = 0.0f; // 0 = abierto, 1 = completamente cerrado

// Declaraciones anticipadas.
void programarSiguienteParpadeo();
void dibujarOjos(float cantidadParpado);
void dibujarOjo(const Ojo &ojo, float cantidadParpado);
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

    if (ojosCerrandose) {
      progresoParpado = progresoFase;
      if (faseTranscurrida >= DURACION_PARPADEO) {
        // Cambia a la fase de apertura.
        ojosCerrandose = false;
        inicioFaseParpadeo = now;
      }
    } else {
      progresoParpado = 1.0f - progresoFase;
      if (faseTranscurrida >= DURACION_PARPADEO) {
        estaParpadeando = false;
        progresoParpado = 0.0f;
        programarSiguienteParpadeo();
      }
    }
  }

  dibujarOjos(progresoParpado);
  delay(16); // Refresco aproximado de 60 FPS
}

void programarSiguienteParpadeo() {
  uint32_t intervalo = random(INTERVALO_PARPADEO_MIN, INTERVALO_PARPADEO_MAX);
  proximoParpadeoEn = millis() + intervalo;
}

void dibujarOjos(float cantidadParpado) {
  dibujarOjo(ojoIzquierdo, cantidadParpado);
  dibujarOjo(ojoDerecho, cantidadParpado);
}

void dibujarOjo(const Ojo &ojo, float cantidadParpado) {
  // Limpia la región del ojo antes de dibujar para evitar sombras.
  int16_t mitadAncho = ANCHO_OJO / 2;
  int16_t mitadAlto = ALTO_OJO / 2;
  gfx->fillRect(ojo.centroX - mitadAncho - 2, ojo.centroY - mitadAlto - 2,
                ANCHO_OJO + 4, ALTO_OJO + 4, COLOR_FONDO);

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
