/*
  Simulador de ECG — MODO VECTOR + PÁGINA WEB POR WiFi — ESP32-CAM (AI-Thinker)
  ----------------------------------------------------------------------------
  MISMO HARDWARE que el simulador de 2 canales: no se cambia nada en la protoboard.
    Canal LA: IO14 -> R1 1k -> [C1 100n] -> R2 10k -> [C2 10n] = TP_LA -> R3 220k -> LA -> 200 ohm -> GND
    Canal LL: IO13 -> R1 1k -> [C1 100n] -> R2 10k -> [C2 10n] = TP_LL -> R3 220k -> LL -> 200 ohm -> GND
    RA = GND (referencia de las salidas)      N/RL = GND

  CÓMO SE USA
    1. Esta carpeta trae DOS archivos: este y "pagina.h" (la página web). Deben quedar juntos.
    2. Subir el programa. Placa: "AI Thinker ESP32-CAM" (paquete esp32 3.x). Sin tarjeta microSD.
    3. En el teléfono o el computador, conectarse a la red WiFi  Simulador-ECG  (clave: ecg12345).
    4. Abrir el navegador en  http://192.168.4.1
       En muchos teléfonos la página se abre sola al conectarse a la red.

  LA IDEA (igual que el modo vector)
    El corazón es UNA flecha H(t) en 3D:  x -> brazo izquierdo   y -> pies   z -> pecho
    Cada electrodo "ve" la proyección de esa flecha:   phi = L · H
    Cada borne entrega la diferencia contra RA:
        borne LA = (L_LA - L_RA) · H  = DI          borne LL = (L_LL - L_RA) · H  = DII
    DIII sale sola en el monitor: DII - DI.

  QUÉ CAMBIA RESPECTO DEL MODO VECTOR
    - La placa crea una red WiFi y entrega una página para controlarla.
    - La señal la genera una tarea aparte, una vez por milisegundo. Atender la página no la frena.
    - Hay dos juegos de tablas: la señal sale de uno mientras el otro se recalcula.
    - Los comandos por el Monitor Serie (115200) y los botones siguen funcionando igual:
        0 .. 5 escenario | eje <grados> | ritmo <0-4> | run <0|1> | plot <0-2> | lv <9 números> | ?

  Botones opcionales (a GND): IO15 = ritmo · IO2 = escenario · IO12 = run/stop
*/

#include <math.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "pagina.h"

// ---- WiFi: la placa crea su propia red ----
const bool  USAR_WIFI   = true;             // false = sin red, igual que el modo vector
const char* WIFI_NOMBRE = "Simulador-ECG";
const char* WIFI_CLAVE  = "ecg12345";       // 8 caracteres o más
const int   WIFI_CANAL  = 6;

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
bool  libres = false;               // true = posiciones enviadas desde la página o el PC
float escala = 1.0f;

const int   BPM[]   = {40, 60, 120, 180};
const int   N_MODOS = 5;            // 4 ritmos + calibración
int   modo      = 1;
int   plot      = 0;

const int MAX_RR = 1500;
float hx[MAX_RR], hy[MAX_RR], hz[MAX_RR];      // la flecha del corazón, milisegundo a milisegundo
float mvLA[MAX_RR], mvLL[MAX_RR];              // lo que ve el monitor: DI y DII (mV)
int   largo = 1000, idxR = 400;

// Dos juegos de tablas de salida. La tarea de la señal lee el que está "en uso";
// los cambios se preparan en el otro y al final se intercambian de un golpe.
uint16_t tablaLA[2][MAX_RR], tablaLL[2][MAX_RR];
int      largoT[2] = {1000, 1000}, idxRT[2] = {400, 400};
volatile int  enUso     = 0;
volatile bool corriendo = true;
volatile bool reiniciar = false;    // pide empezar el latido desde el principio
volatile int  idxAct    = 0;        // en qué milisegundo del latido va la salida

// ---- Servidor web ----
// Atiende hasta 4 conexiones a la vez y nunca se queda esperando a una sola.
struct Visita { WiFiClient c; char linea[200]; uint8_t n; uint8_t saltos; bool lista; uint32_t t0; };
const int N_VISITAS = 4;
Visita     visitas[N_VISITAS];
WiFiServer servidor(80);
DNSServer  dns;
bool       wifiListo = false;

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
  if (libres) Serial.println("#   Electrodos: posicion libre (enviada desde la pagina o el PC)");
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
  int n = 1 - enUso;                                // se escribe en el juego que NO está saliendo
  escala = 1.0f;
  if (modo == 4) {                                  // calibración: 1 mV en ambos bornes, 1 Hz
    for (int i = 0; i < largo; i++) {
      mvLA[i] = mvLL[i] = (i < 500) ? 1.0f : 0.0f;
      tablaLA[n][i] = mV_a_duty(mvLA[i], ATEN_LA);
      tablaLL[n][i] = mV_a_duty(mvLL[i], ATEN_LL);
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
      tablaLA[n][i] = mV_a_duty(mvLA[i], ATEN_LA);
      tablaLL[n][i] = mV_a_duty(mvLL[i], ATEN_LL);
    }
  }
  largoT[n] = largo; idxRT[n] = idxR;
  enUso = n;                                        // desde el próximo milisegundo sale la tabla nueva
  informar();
}

void ayuda() {
  Serial.println("# Comandos: 0-5 escenario | eje <grados> | ritmo <0-4> | run <0|1> | plot <0-2> | ?");
  for (int i = 0; i < N_ESC; i++) Serial.printf("#   %d %s\n", i, ESC[i].nombre);
}

// Los comandos llegan por tres caminos (Monitor Serie, botones y página web) y todos pasan por aquí
void ejecutar(char* s) {
  if (s[0] >= '0' && s[0] <= '5' && s[1] == 0) {
    aplicarEscenario(s[0] - '0'); if (modo == 4) modo = 1;
    construirCorazon(); proyectar();
  } else if (strncmp(s, "eje", 3) == 0) {
    float e = atof(s + 3);
    if (isfinite(e)) { ejeObj = fmodf(e, 360.0f); if (modo == 4) modo = 1; construirCorazon(); proyectar(); }
  } else if (strncmp(s, "ritmo", 5) == 0) {
    int m = atoi(s + 5);
    if (m >= 0 && m < N_MODOS) { modo = m; construirCorazon(); proyectar(); }
  } else if (strncmp(s, "run", 3) == 0) {
    corriendo = s[3] ? atoi(s + 3) != 0 : !corriendo;
    Serial.println(corriendo ? "# RUN" : "# STOP");
  } else if (strncmp(s, "lv", 2) == 0) {
    float v[9]; char* p = s + 2; int n = 0;
    for (; n < 9; n++) { char* fin; v[n] = strtof(p, &fin); if (fin == p || !isfinite(v[n]) || fabsf(v[n]) > 100.0f) break; p = fin; }
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

// ---------------------------------------------------------------------------
// LA SEÑAL: una tarea aparte que despierta una vez por milisegundo.
// Tiene más prioridad que el resto del programa, así que la página no la retrasa.
// ---------------------------------------------------------------------------
void tareaSenal(void*) {
  TickType_t ultimo = xTaskGetTickCount();
  int idx = 0;
  for (;;) {
    vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(1));
    int n = enUso;
    if (reiniciar) { reiniciar = false; idx = 0; }
    if (idx >= largoT[n]) idx = 0;
    if (corriendo) {
      ledcWrite(PIN_LA, tablaLA[n][idx]);
      ledcWrite(PIN_LL, tablaLL[n][idx]);
      int d = idx - idxRT[n];
      digitalWrite(LED_PLACA, (d >= 0 && d < 60) ? LOW : HIGH);
      idx = (idx + 1) % largoT[n];
    } else {
      ledcWrite(PIN_LA, mV_a_duty(0, ATEN_LA));
      ledcWrite(PIN_LL, mV_a_duty(0, ATEN_LL));
      digitalWrite(LED_PLACA, HIGH);
    }
    idxAct = idx;
  }
}

// ---------------------------------------------------------------------------
// LA PÁGINA WEB
//   /             -> la página (guardada en pagina.h)
//   /api/estado   -> cómo está el simulador ahora
//   /api/cmd?c=X  -> ejecuta el comando X (los mismos del Monitor Serie) y responde el estado
//   lo demás      -> lleva a la página (por eso en los teléfonos se abre sola)
// ---------------------------------------------------------------------------
void cabecera(WiFiClient& c, const char* resultado, const char* tipo, size_t largoCuerpo, const char* extra) {
  c.printf("HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nCache-Control: no-store\r\nConnection: close\r\n%s\r\n",
           resultado, tipo, (unsigned)largoCuerpo, extra);
}

void enviar(WiFiClient& c, const char* datos, size_t n) {       // de a trozos, para que quepa la página entera
  while (n > 0) {
    size_t k = n > 1460 ? 1460 : n;
    if (c.write((const uint8_t*)datos, k) != k) return;
    datos += k; n -= k;
  }
}

void enviarEstado(WiFiClient& c) {
  char eje[16] = "null";
  if (!isnan(ejeObj)) snprintf(eje, sizeof eje, "%.1f", ejeObj);
  char j[420];
  int n = snprintf(j, sizeof j,
    "{\"esc\":%d,\"eje\":%s,\"espejo\":%s,\"libres\":%s,\"modo\":%d,\"corriendo\":%s,\"escala\":%.4f,\"idx\":%d,\"n\":%d,"
    "\"L\":[%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f]}",
    esc, eje, espejo ? "true" : "false", libres ? "true" : "false", modo, corriendo ? "true" : "false", escala, (int)idxAct, largo,
    L[0][0], L[0][1], L[0][2], L[1][0], L[1][1], L[1][2], L[2][0], L[2][1], L[2][2]);
  if (n >= (int)sizeof j) n = sizeof j - 1;
  cabecera(c, "200 OK", "application/json", n, "");
  enviar(c, j, n);
}

// Deja "eje%2075" como "eje 75"
void decodificar(const char* p, char* sal, size_t tope) {
  size_t n = 0;
  for (; *p && *p != '&' && n < tope - 1; p++) {
    if (*p == '+') sal[n++] = ' ';
    else if (*p == '%' && isxdigit((unsigned char)p[1]) && isxdigit((unsigned char)p[2])) {
      char h[3] = {p[1], p[2], 0};
      sal[n++] = (char)strtol(h, nullptr, 16); p += 2;
    } else sal[n++] = *p;
  }
  sal[n] = 0;
}

void responder(WiFiClient& c, char* linea) {        // linea = "GET /ruta HTTP/1.1"
  char* ruta = strchr(linea, ' ');
  if (!ruta) return;
  ruta++;
  char* fin = strchr(ruta, ' ');
  if (fin) *fin = 0;

  if (strcmp(ruta, "/") == 0 || strncmp(ruta, "/?", 2) == 0) {
    size_t n = strlen_P(PAGINA);
    cabecera(c, "200 OK", "text/html; charset=utf-8", n, "");
    enviar(c, PAGINA, n);
  } else if (strcmp(ruta, "/api/estado") == 0) {
    enviarEstado(c);
  } else if (strncmp(ruta, "/api/cmd?c=", 11) == 0) {
    char cmd[128];
    decodificar(ruta + 11, cmd, sizeof cmd);
    ejecutar(cmd);
    enviarEstado(c);
  } else if (strcmp(ruta, "/favicon.ico") == 0) {
    cabecera(c, "204 No Content", "text/plain", 0, "");
  } else {
    cabecera(c, "302 Found", "text/plain", 0, "Location: http://192.168.4.1/\r\n");
  }
}

void atenderWeb() {
  WiFiClient nuevo = servidor.accept();
  if (nuevo) {
    int libre = -1;
    for (int i = 0; i < N_VISITAS && libre < 0; i++) if (!visitas[i].c) libre = i;
    if (libre < 0) nuevo.stop();
    else { Visita& v = visitas[libre]; v.c = nuevo; v.n = 0; v.saltos = 0; v.lista = false; v.t0 = millis(); }
  }
  for (int i = 0; i < N_VISITAS; i++) {
    Visita& v = visitas[i];
    if (!v.c) continue;
    if (millis() - v.t0 > 3000) { v.c.stop(); continue; }          // conexión abierta que no pide nada
    for (int tope = 800; tope > 0 && v.c.available(); tope--) {
      char ch = v.c.read();
      if (!v.lista) {                                              // solo interesa la primera línea
        if (ch == '\n') v.lista = true;
        else if (ch != '\r' && v.n < sizeof(v.linea) - 1) v.linea[v.n++] = ch;
      }
      if (ch == '\n') v.saltos++; else if (ch != '\r') v.saltos = 0;
      if (v.saltos == 2) {                                         // línea en blanco: terminó el pedido
        v.linea[v.n] = 0;
        responder(v.c, v.linea);
        v.c.stop();
        break;
      }
    }
  }
}

void setup() {
  Serial.setTxBufferSize(2048);                     // para que los avisos no frenen el programa
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
  Serial.printf("# PWM OK: %u Hz, %u bits | MODO VECTOR v2 + WiFi | eje normal = %+.0f grados\n",
                (unsigned)PWM_FREQ, (unsigned)PWM_BITS, EJE_BASE);

  aplicarEscenario(0);
  construirCorazon();
  proyectar();
  ayuda();

  // La señal parte antes que el WiFi: si la red falla, el simulador funciona igual.
  xTaskCreatePinnedToCore(tareaSenal, "senal", 4096, nullptr, 10, nullptr, 1);

  if (USAR_WIFI) {
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(WIFI_NOMBRE, WIFI_CLAVE, WIFI_CANAL, 0, 4)) {
      WiFi.setTxPower(WIFI_POWER_8_5dBm);           // poca potencia: alcanza para una sala y mete menos ruido
      dns.start(53, "*", WiFi.softAPIP());          // cualquier dirección que se escriba lleva a la página
      servidor.begin();
      wifiListo = true;
      Serial.printf("# WiFi listo. Red \"%s\", clave \"%s\". Abrir http://%s\n",
                    WIFI_NOMBRE, WIFI_CLAVE, WiFi.softAPIP().toString().c_str());
    } else {
      Serial.println("# AVISO: no se pudo crear la red WiFi. El simulador sigue funcionando por cable.");
    }
  }
  Serial.println("# Simulador listo.");
}

void loop() {
  static bool pM = false, pE = false, pR = false;
  static uint32_t tM = 0, tE = 0, tR = 0, tPlot = 0;

  leerSerie();
  if (presionado(BTN_RITMO, pM, tM)) { modo = (modo + 1) % N_MODOS; construirCorazon(); proyectar(); reiniciar = true; }
  if (presionado(BTN_ESC,   pE, tE)) {
    aplicarEscenario((esc + 1) % N_ESC); if (modo == 4) modo = 1;
    construirCorazon(); proyectar(); reiniciar = true;
  }
  if (presionado(BTN_RUN,   pR, tR)) { corriendo = !corriendo; Serial.println(corriendo ? "# RUN" : "# STOP"); }

  if (wifiListo) { dns.processNextRequest(); atenderWeb(); }

  if (plot && corriendo && millis() - tPlot >= 10) {              // 100 muestras por segundo al PC
    tPlot = millis();
    int i = idxAct;
    if (i < largo) {
      Serial.printf("DI:%.3f,DII:%.3f,DIII:%.3f", mvLA[i], mvLL[i], mvLL[i] - mvLA[i]);
      if (plot == 2) Serial.printf(",HX:%.3f,HY:%.3f,HZ:%.3f", hx[i], hy[i], hz[i]);
      Serial.println();
    }
  }
  delay(1);
}
