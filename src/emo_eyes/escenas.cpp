#include "escenas.h"
#include "funciones.h"
#include "secuencias.h"

// --- PROTOTIPOS de overlays (se llaman antes de su definición) ---
void overlay_frio_cristales(unsigned long now);
void overlay_frio_vaho(unsigned long now);

void overlay_calor_gotas(unsigned long now);

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
      dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
      dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      break;

    case ESCENA_DESPERTAR:
    case ESCENA_DESPERTAR_RENDER:
      dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
      dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      break;

    case ESCENA_PRE_DORMIR: {
      if (est.dormirBostezoFrame) {
        dibujarOjoBostezoIzq(baseLeftX,  leftY,  EYE_W, est.leftEyeH);
        dibujarOjoBostezoDer(baseRightX, rightY, EYE_W, est.rightEyeH);
      } else {
        dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
        dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      }
      if (est.dormirZzzActivo) {
        zzz_render();
      }
    } break;

    // ====== NUEVOS CASES VISIBLES ======
    case ESCENA_FRIO:
      // igual que NORMAL (los efectos se dibujan abajo como overlay)
      dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
      dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      overlay_frio_cristales(millis());
      overlay_frio_vaho(millis());
      break;

    case ESCENA_CALOR:
      // igual que NORMAL (los efectos se dibujan abajo como overlay)
      dibujarOjoNormal(baseLeftX,  leftY,  EYE_W, est.leftEyeH,  est.lidProgress);
      dibujarOjoNormal(baseRightX, rightY, EYE_W, est.rightEyeH, est.lidProgress);
      overlay_calor_gotas(millis());
      break;

    case ESCENA_NINGUNA:
    default:
      // nada
      break;
  }


  // IMPORTANTE: empujar al final, después de overlays
  empujarStageACentro();
}


// ====== (lo demás igual que ya tenías) ======

extern TFT_eSprite stage;

// =================== ESCENA FRÍO ===================

// Cristales / copos cayendo y girando
struct Copo { bool on; float x,y; float vy, vx; uint8_t r, kind; };
static Copo COP[24];
static unsigned long copoSpawn = 0;

void overlay_frio_cristales(unsigned long now) {
  // spawn cada ~160ms
  if (now >= copoSpawn) {
    for (int i = 0; i < (int)(sizeof(COP)/sizeof(Copo)); ++i) if (!COP[i].on) {
      COP[i].on = true;
      COP[i].x = random(0, STAGE_W);
      COP[i].y = -4;
      COP[i].vy = 0.6f + (random(0, 12)/10.0f);   // 0.6..1.8 px/frame aprox
      COP[i].vx = (random(0,2) ? -0.4f : 0.4f);
      COP[i].r = (uint8_t)random(1, 3);
      COP[i].kind = (uint8_t)random(0, 3);
      break;
    }
    copoSpawn = now + 160;
  }

  for (int i = 0; i < (int)(sizeof(COP)/sizeof(Copo)); ++i) if (COP[i].on) {
    // dibujo simple del copo
    uint16_t col = stage.color565(210, 230, 255);
    int x = (int)COP[i].x, y = (int)COP[i].y;
    switch (COP[i].kind) {
      case 0: stage.drawPixel(x, y, col); break;
      case 1: stage.drawFastHLine(x-1, y, 3, col); stage.drawFastVLine(x, y-1, 3, col); break;
      case 2: stage.drawLine(x-2, y, x+2, y, col); stage.drawLine(x, y-2, x, y+2, col); break;
    }
    // update
    COP[i].x += COP[i].vx * 0.9f;
    COP[i].y += COP[i].vy * 0.9f;
    if (COP[i].y > STAGE_H + 6) COP[i].on = false;
  }
}

// Vaho (aliento frío) saliendo de “la boca”
struct Puff { bool on; float x,y; float vx, vy; uint8_t r; unsigned long born, life; };
static Puff PU[6];

void overlay_frio_vaho(unsigned long now) {
  // cada ~700ms un puff
  static unsigned long nextP = 0;
  if (now >= nextP) {
    for (int i = 0; i < (int)(sizeof(PU)/sizeof(Puff)); ++i) if (!PU[i].on) {
      PU[i].on = true;
      PU[i].x = STAGE_W/2 + random(-4,5);
      PU[i].y = STAGE_H - 10;
      PU[i].vx = (random(0,2) ? -0.2f : 0.2f);
      PU[i].vy = -0.6f - (random(0,6)/10.0f);
      PU[i].r = 3;
      PU[i].born = now;
      PU[i].life = 900;
      break;
    }
    nextP = now + 700;
  }
  // render/update (anillos para “semi-transparencia” fake)
  for (int i = 0; i < (int)(sizeof(PU)/sizeof(Puff)); ++i) if (PU[i].on) {
    unsigned long age = now - PU[i].born;
    float k = (float)age / (float)PU[i].life; if (k > 1) k = 1;
    int r = PU[i].r + (int)(6*k);
    uint16_t c = stage.color565(200, 220, 240);
    // anillo: círculo claro + agujero BG para dar sensación de niebla
    stage.drawCircle((int)PU[i].x, (int)PU[i].y, r, c);
    if (r > 2) stage.drawCircle((int)PU[i].x, (int)PU[i].y, r-2, BG_COLOR);

    PU[i].x += PU[i].vx;
    PU[i].y += PU[i].vy;
    if (age >= PU[i].life || PU[i].y < 6) PU[i].on = false;
  }
}


// =================== ESCENA CALOR ===================

// Gotas de sudor cayendo lentamente
struct Gota { 
  bool on; 
  float x, y; 
  float vy, vx;
  uint8_t len, wobble;
  uint8_t r;                 // <-- radio de la cabeza (1..3)
  unsigned long born, life;
};
static Gota GOT[32];
static unsigned long gotaSpawn = 0;

void overlay_calor_gotas(unsigned long now) {
  // Spawnea una gota nueva aprox cada 120ms si hay slot libre
  if (now >= gotaSpawn) {
    for (int i = 0; i < (int)(sizeof(GOT)/sizeof(Gota)); ++i) if (!GOT[i].on) {
      GOT[i].on   = true;
      GOT[i].x    = random(0, STAGE_W);
      GOT[i].y    = -6;
      GOT[i].vy   = 0.35f + (random(0,9)/10.0f);      // 0.35..1.25
      GOT[i].vx   = (random(0,2) ? 0.10f : -0.10f);   // leve deriva
      GOT[i].len  = (uint8_t)random(3, 7);            // colita corta
      GOT[i].wobble = (uint8_t)random(12, 22);        // zig-zag
      GOT[i].r    = (uint8_t)random(1, 4);            // tamaño 1..3
      GOT[i].born = now;
      GOT[i].life = 4000;
      break;
    }
    gotaSpawn = now + 120;
  }

  // Paleta "sudor"
  const uint16_t base  = stage.color565(110, 180, 255); // cuerpo
  const uint16_t mid   = stage.color565(140, 205, 255); // cola (más claro)
  const uint16_t shine = stage.color565(210, 245, 255); // brillo

  for (int i = 0; i < (int)(sizeof(GOT)/sizeof(Gota)); ++i) if (GOT[i].on) {
    int x = (int)GOT[i].x;
    int y = (int)GOT[i].y;
    int r = (int)GOT[i].r;

    // === RENDER ===
    // 1) Cola con pseudo-degradé: mitad inferior en 'base', mitad superior en 'mid'
    int tail = (int)GOT[i].len + (r - 1);                // un poco más según tamaño
    int tailTop = y - tail + 1;
    if (tail > 0) {
      int half = tail / 2;
      // parte inferior (más “pesada”)
      stage.fillRect(x - (r-1), y - half + 1, (r*2 - 1), half, base);
      // parte superior (más clara)
      if (half > 0) stage.fillRect(x - (r-1), tailTop, (r*2 - 1), max(0, tail - half), mid);
    }

    // 2) Cabeza redondeada rellena
    stage.fillCircle(x, y, r, base);

    // 3) Brillo lateral (1 px) arriba-izquierda
    if (r >= 2) {
      int bx = x - (r - 1);
      int by = y - (r - 1);
      if (bx >= 0 && bx < STAGE_W && by >= 0 && by < STAGE_H)
        stage.drawPixel(bx, by, shine);
    }

    // === UPDATE ===
    // zig-zag suave tipo triángulo
    float period = (float)(GOT[i].wobble * 10);
    float t = (float)((now - GOT[i].born) % (unsigned long)period) / period;
    float tri = (t < 0.5f) ? (t*2.0f) : (2.0f - t*2.0f);  // 0..1..0
    float lateral = (tri - 0.5f) * (0.2f + 0.06f * r);    // amplitud escala con tamaño

    // caída con leve “gravedad” y límite
    GOT[i].vy = min(GOT[i].vy + 0.01f, 1.6f + 0.08f * r);

    GOT[i].x += GOT[i].vx + lateral;
    GOT[i].y += GOT[i].vy;

    // kill conditions
    if (GOT[i].y > STAGE_H + 8) GOT[i].on = false;
    else if (now - GOT[i].born > GOT[i].life) GOT[i].on = false;
  }
}

