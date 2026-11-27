/*
  Simulador de ECG — MODO VECTOR v2 (idea de José Tomás) — ESP32-CAM (AI-Thinker)
  ------------------------------------------------------------------------------
  MISMO HARDWARE que el simulador de 2 canales: no se cambia nada en la protoboard.
    Canal LA: IO14 -> R1 1k -> [C1 100n] -> R2 10k -> [C2 10n] = TP_LA -> R3 220k -> LA -> 200 ohm -> GND
    Canal LL: IO13 -> R1 1k -> [C1 100n] -> R2 10k -> [C2 10n] = TP_LL -> R3 220k -> LL -> 200 ohm -> GND
    RA = GND (referencia de las salidas)      N/RL = GND

  LA IDEA
    El corazón es UNA flecha H(t) en 3D (vectocardiograma):
        x -> hacia el brazo izquierdo     y -> hacia los pies     z -> hacia el pecho
    Cada electrodo "ve" la proyección de esa flecha:   phi = L · H
        L = vector de derivación del electrodo (depende de DÓNDE está puesto en el cuerpo)
    El micro decide qué electrodo queda en cada borne y saca la diferencia contra RA:
        borne LA = (L_LA - L_RA) · H  = DI que verá el monitor
        borne LL = (L_LL - L_RA) · H  = DII que verá el monitor       (DIII sale sola: DII - DI)
    Como el monitor solo mide diferencias, con 2 canales alcanza para 3 electrodos
    puestos en CUALQUIER parte del cuerpo.

  COMANDOS (Monitor Serie a 115200, con "Nueva línea")
    0 .. 5        escenario (también con el botón de IO2):
                    0 Normal (eje +41°)            3 Cables RA y LA invertidos
                    1 Desviación izquierda (-45°)  4 Cables LA y LL invertidos
                    2 Desviación derecha (+115°)   5 Dextrocardia
    eje 75        gira el corazón: el eje del QRS queda en 75° (cualquier ángulo)
    ritmo 2       0 = 40 lpm · 1 = 60 · 2 = 120 · 3 = 180 · 4 = calibración (también botón IO15)
    lv <9 números>  vectores de derivación de RA, LA y LL (x y z de cada uno).
                    Los manda el programa del PC (vector_ecg.py) al mover los electrodos.
    plot 1        envía DI, DII, DIII (sirve para el Serial Plotter del Arduino IDE)
    plot 2        además envía la flecha HX, HY, HZ (lo usa vector_ecg.py)   ·   plot 0 = nada
    ?             ayuda

  Botones opcionales (a GND): IO15 = ritmo · IO2 = escenario · IO12 = run/stop
  Placa: "AI Thinker ESP32-CAM" (paquete esp32 3.x). Sin tarjeta microSD.
*/

#include <math.h>

const int PIN_LA     = 14;
const int PIN_LL     = 13;
const int BTN_RITMO  = 15;
const int BTN_ESC    = 2;
const int BTN_RUN    = 12;
const int LED_PLACA  = 33;         // LED rojo integrado (activo en LOW)
const int FLASH_CAM  = 4;

// ---- PWM ----
const uint32_t OPC_FREQ[] = {62500, 39062, 19531};
uint32_t PWM_FREQ = 0;
const uint8_t  PWM_BITS = 10;
const int      PWM_MAX  = (1 << PWM_BITS) - 1;

// ---- Circuito ----
const float VDD      = 3.3f;
const float G_FILTRO = 0.952f;
// Calibración por canal: medir R3 y R4 de cada canal y poner (R3 + R4) / R4
const float ATEN_LA  = 1101.0f;
const float ATEN_LL  = 1101.0f;
// Línea base al CENTRO del rango: las ondas pueden ser positivas o negativas.
const float TP_BASE   = 1.55f;
// Máximo que cabe en un borne (mV). Si un escenario pide más, se reduce todo en proporción.
const float LIMITE_MV = 1.35f;

// ---- El corazón como vector ----
// Alturas del código de 2 canales (mV). De ahí sale la flecha de cada onda:
//   x = DI        y = (2·DII − DI) / √3        z = componente hacia el pecho (ilustrativa)
//                       P      Q       R      S       T
const float AMP_DI[]  = {0.10f, -0.03f, 0.80f, -0.05f, 0.35f};
const float AMP_DII[] = {0.15f, -0.12f, 1.00f, -0.30f, 0.20f};
const float AMP_Z[]   = {0.03f,  0.08f, -0.30f, -0.25f, 0.15f};
float V[5][3];
float EJE_BASE = 0;                 // eje del QRS normal (grados), se calcula en setup

// ---- Los electrodos ----
// Posiciones estándar: vértices del triángulo de Einthoven (plano frontal, z = 0)
const int E_RA = 0, E_LA = 1, E_LL = 2;
const float U[3][3] = {{-0.5f, -0.28868f, 0.0f}, {0.5f, -0.28868f, 0.0f}, {0.0f, 0.57735f, 0.0f}};
const char* const NOMBRE_E[3] = {"RA", "LA", "LL"};

// cable[b] = qué electrodo del cuerpo quedó conectado al borne b del monitor
struct Escenario { const char* nombre; float eje; bool espejo; uint8_t cable[3]; };
const Escenario ESC[] = {
  {"Normal",                              NAN,   false, {0, 1, 2}},
  {"Desviacion del eje a la izquierda",   -45,   false, {0, 1, 2}},
  {"Desviacion del eje a la derecha",     115,   false, {0, 1, 2}},
  {"Cables RA y LA invertidos",           NAN,   false, {1, 0, 2}},
  {"Cables LA y LL invertidos",           NAN,   false, {0, 2, 1}},
  {"Dextrocardia",                        NAN,   true,  {0, 1, 2}},
};
const int N_ESC = 6;

// ---- Estado ----
int   esc    = 0;
float ejeObj = NAN;                 // eje pedido para el QRS (NAN = el del corazón normal)
bool  espejo = false;
float L[3][3];                      // vector de derivación del electrodo que está en cada borne
bool  libres = false;               // true = posiciones enviadas desde el PC
float escala = 1.0f;

const int   BPM[]   = {40, 60, 120, 180};
const int   N_MODOS = 5;            // 4 ritmos + calibración
int   modo      = 1;
bool  corriendo = true;
int   plot      = 0;

const int MAX_RR = 1500;
uint16_t tablaLA[MAX_RR], tablaLL[MAX_RR];
float    hx[MAX_RR], hy[MAX_RR], hz[MAX_RR];   // la flecha del corazón, milisegundo a milisegundo
float    mvLA[MAX_RR], mvLL[MAX_RR];           // lo que ve el monitor: DI y DII (mV)
int largo = 1000, idxR = 400;

hw_timer_t* timer = nullptr;
volatile uint32_t ticks = 0;
uint32_t hechos = 0;
void ARDUINO_ISR_ATTR onTimer() { ticks = ticks + 1; }

uint16_t mV_a_duty(float mv, float aten) {
  float tp = TP_BASE + mv * aten / 1000.0f;
  int   c  = (int)roundf(tp / (G_FILTRO * VDD) * PWM_MAX);
  return (uint16_t)constrain(c, 0, PWM_MAX);
}

float g(float t, float c, float w) { float x = (t - c) / w; return expf(-0.5f * x * x); }

void aplicarEscenario(int n) {
  esc = n; ejeObj = ESC[n].eje; espejo = ESC[n].espejo; libres = false;
  for (int b = 0; b < 3; b++)
    for (int j = 0; j < 3; j++) L[b][j] = U[ESC[n].cable[b]][j];
}

// Paso 1: la flecha H(t) de un latido completo, con el corazón girado o reflejado
void construirCorazon() {
  if (modo == 4) {
    largo = 1000; idxR = 0;
    for (int i = 0; i < largo; i++) hx[i] = hy[i] = hz[i] = 0;
    return;
  }
  float giro = isnan(ejeObj) ? 0.0f : (ejeObj - EJE_BASE) * (float)M_PI / 180.0f;
  float co = cosf(giro), si = sinf(giro);
  float v[5][3];
  for (int k = 0; k < 5; k++) {
    v[k][0] = V[k][0] * co - V[k][1] * si;
    v[k][1] = V[k][0] * si + V[k][1] * co;
    v[k][2] = V[k][2];
    if (espejo) v[k][0] = -v[k][0];
  }
  int RR = 60000 / BPM[modo];
  float k = sqrtf(RR / 1000.0f), R = 0.40f * RR;
  float centro[5] = {R - 190 * k, R - 25, R, R + 25, R + 240 * k};
  float ancho[5]  = {22 * k,      8,      9, 8,      40 * k};
  largo = RR; idxR = (int)R;

  for (int i = 0; i < RR; i++) {
    float x = 0, y = 0, z = 0;
    for (int w = 0; w < 5; w++) {
      float a = g(i, centro[w], ancho[w]);
      x += a * v[w][0]; y += a * v[w][1]; z += a * v[w][2];
    }
    hx[i] = x; hy[i] = y; hz[i] = z;
  }
  float x0 = hx[0], xN = hx[RR - 1], y0 = hy[0], yN = hy[RR - 1], z0 = hz[0], zN = hz[RR - 1];
  for (int i = 0; i < RR; i++) {                    // empalme suave entre latidos
    float f = i / (float)(RR - 1);
    hx[i] -= x0 + (xN - x0) * f;
    hy[i] -= y0 + (yN - y0) * f;
    hz[i] -= z0 + (zN - z0) * f;
  }
}

void informar() {
  Serial.printf("# Escenario %d: %s", esc, ESC[esc].nombre);
  if (!isnan(ejeObj) && !(ejeObj == ESC[esc].eje)) Serial.printf(" + eje %.0f", ejeObj);
  if (libres) Serial.print(" + electrodos libres");
  if (modo == 4) Serial.println(" | CALIBRACION 1 Hz / 1 mV");
  else Serial.printf(" | %d lpm\n", BPM[modo]);
  if (libres) Serial.println("#   Electrodos: posicion libre (enviada desde el PC)");
  else Serial.printf("#   Electrodos: borne RA <- %s, borne LA <- %s, borne LL <- %s\n",
                     NOMBRE_E[ESC[esc].cable[0]], NOMBRE_E[ESC[esc].cable[1]], NOMBRE_E[ESC[esc].cable[2]]);
  if (modo != 4) {
    float dI = mvLA[idxR], dII = mvLL[idxR];
    float eje = atan2f((2 * dII - dI) / sqrtf(3.0f), dI) * 180.0f / (float)M_PI;
    Serial.printf("#   R en el monitor: DI %+.2f  DII %+.2f  DIII %+.2f mV | eje aparente %+.0f grados\n",
                  dI, dII, dII - dI, eje);
  }
  if (escala < 1.0f)
    Serial.printf("# AVISO: senal reducida al %.0f %% para que quepa en la salida\n", escala * 100.0f);
}

// Paso 2: lo que entrega cada borne según dónde están los electrodos
void proyectar() {
  escala = 1.0f;
  if (modo == 4) {                                  // calibración: 1 mV en ambos bornes, 1 Hz
    for (int i = 0; i < largo; i++) {
      mvLA[i] = mvLL[i] = (i < 500) ? 1.0f : 0.0f;
      tablaLA[i] = mV_a_duty(mvLA[i], ATEN_LA);
      tablaLL[i] = mV_a_duty(mvLL[i], ATEN_LL);
    }
  } else {
    float a[3], b[3], pico = 0;
    for (int j = 0; j < 3; j++) { a[j] = L[E_LA][j] - L[E_RA][j]; b[j] = L[E_LL][j] - L[E_RA][j]; }
    for (int i = 0; i < largo; i++) {
      mvLA[i] = a[0] * hx[i] + a[1] * hy[i] + a[2] * hz[i];
      mvLL[i] = b[0] * hx[i] + b[1] * hy[i] + b[2] * hz[i];
      pico = fmaxf(pico, fmaxf(fabsf(mvLA[i]), fabsf(mvLL[i])));
    }
    if (pico > LIMITE_MV) escala = LIMITE_MV / pico;
    for (int i = 0; i < largo; i++) {
      mvLA[i] *= escala; mvLL[i] *= escala;
      tablaLA[i] = mV_a_duty(mvLA[i], ATEN_LA);
      tablaLL[i] = mV_a_duty(mvLL[i], ATEN_LL);
    }
  }
  informar();
  hechos = ticks;                                   // descarta el tiempo usado en recalcular
}

void ayuda() {
  Serial.println("# Comandos: 0-5 escenario | eje <grados> | ritmo <0-4> | plot <0-2> | ?");
  for (int i = 0; i < N_ESC; i++) Serial.printf("#   %d %s\n", i, ESC[i].nombre);
}

void ejecutar(char* s) {
  if (s[0] >= '0' && s[0] <= '5' && s[1] == 0) {
    aplicarEscenario(s[0] - '0'); if (modo == 4) modo = 1;
    construirCorazon(); proyectar();
  } else if (strncmp(s, "eje", 3) == 0) {
    ejeObj = atof(s + 3); if (modo == 4) modo = 1;
    construirCorazon(); proyectar();
  } else if (strncmp(s, "ritmo", 5) == 0) {
    int m = atoi(s + 5);
    if (m >= 0 && m < N_MODOS) { modo = m; construirCorazon(); proyectar(); }
  } else if (strncmp(s, "lv", 2) == 0) {
    float v[9]; char* p = s + 2; int n = 0;
    for (; n < 9; n++) { char* fin; v[n] = strtof(p, &fin); if (fin == p) break; p = fin; }
    if (n == 9) {
      for (int b = 0; b < 3; b++) for (int j = 0; j < 3; j++) L[b][j] = v[b * 3 + j];
      libres = true; proyectar();
    } else Serial.println("# lv necesita 9 numeros: x y z de RA, LA y LL");
  } else if (strncmp(s, "plot", 4) == 0) {
    plot = s[4] ? atoi(s + 4) : (plot ? 0 : 1);
  } else ayuda();
}

void leerSerie() {
  static char buf[128]; static int n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') { if (n < 127) buf[n++] = c; continue; }
    if (n == 0) continue;
    buf[n] = 0; n = 0;
    ejecutar(buf);
  }
}

bool presionado(int pin, bool& previo, uint32_t& t0) {
  bool ahora = digitalRead(pin) == LOW, flanco = false;
  if (ahora && !previo && millis() - t0 > 200) { flanco = true; t0 = millis(); }
  previo = ahora;
  return flanco;
}

void setup() {
  Serial.setTxBufferSize(2048);                     // para que los avisos no frenen la señal
  Serial.begin(115200);
  delay(500);
  pinMode(FLASH_CAM, OUTPUT); digitalWrite(FLASH_CAM, LOW);
  pinMode(LED_PLACA, OUTPUT); digitalWrite(LED_PLACA, HIGH);
  pinMode(BTN_RITMO, INPUT_PULLUP);
  pinMode(BTN_ESC,   INPUT_PULLUP);
  pinMode(BTN_RUN,   INPUT_PULLUP);

  for (int k = 0; k < 5; k++) {
    V[k][0] = AMP_DI[k];
    V[k][1] = (2 * AMP_DII[k] - AMP_DI[k]) / sqrtf(3.0f);
    V[k][2] = AMP_Z[k];
  }
  EJE_BASE = atan2f(V[2][1], V[2][0]) * 180.0f / (float)M_PI;

  for (int i = 0; i < 3 && PWM_FREQ == 0; i++) {
    if (ledcAttach(PIN_LA, OPC_FREQ[i], PWM_BITS)) {
      if (ledcAttach(PIN_LL, OPC_FREQ[i], PWM_BITS)) PWM_FREQ = OPC_FREQ[i];
      else ledcDetach(PIN_LA);
    }
  }
  if (PWM_FREQ == 0) { Serial.println("ERROR: no se pudo configurar el PWM"); while (true) delay(1000); }
  Serial.printf("# PWM OK: %u Hz, %u bits | MODO VECTOR v2 | eje normal = %+.0f grados\n",
                (unsigned)PWM_FREQ, (unsigned)PWM_BITS, EJE_BASE);

  aplicarEscenario(0);
  construirCorazon();
  proyectar();
  ayuda();
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000, true, 0);
  Serial.println("# Simulador listo.");
}

void loop() {
  static bool pM = false, pE = false, pR = false;
  static uint32_t tM = 0, tE = 0, tR = 0;
  static int idx = 0;

  leerSerie();
  if (presionado(BTN_RITMO, pM, tM)) { modo = (modo + 1) % N_MODOS; idx = 0; construirCorazon(); proyectar(); }
  if (presionado(BTN_ESC,   pE, tE)) {
    aplicarEscenario((esc + 1) % N_ESC); if (modo == 4) modo = 1;
    idx = 0; construirCorazon(); proyectar();
  }
  if (presionado(BTN_RUN,   pR, tR)) { corriendo = !corriendo; Serial.println(corriendo ? "# RUN" : "# STOP"); }
  if (idx >= largo) idx = 0;

  while (hechos != ticks) {                         // una vez por cada milisegundo
    hechos++;
    if (corriendo) {
      ledcWrite(PIN_LA, tablaLA[idx]);
      ledcWrite(PIN_LL, tablaLL[idx]);
      int d = idx - idxR;
      digitalWrite(LED_PLACA, (d >= 0 && d < 60) ? LOW : HIGH);
      if (plot && hechos % 10 == 0) {               // 100 muestras por segundo al PC
        Serial.printf("DI:%.3f,DII:%.3f,DIII:%.3f", mvLA[idx], mvLL[idx], mvLL[idx] - mvLA[idx]);
        if (plot == 2) Serial.printf(",HX:%.3f,HY:%.3f,HZ:%.3f", hx[idx], hy[idx], hz[idx]);
        Serial.println();
      }
      idx = (idx + 1) % largo;
    } else {
      ledcWrite(PIN_LA, mV_a_duty(0, ATEN_LA));
      ledcWrite(PIN_LL, mV_a_duty(0, ATEN_LL));
      digitalWrite(LED_PLACA, HIGH);
    }
  }
}
