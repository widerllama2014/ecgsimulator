/*
  Simulador de ECG de 3 derivaciones con DOS CANALES — ESP32-CAM (AI-Thinker)
  ----------------------------------------------------------------------------
  Idea (pizarra del profe): dos señales independientes, V1 y V2, sobre una referencia.
    Canal LA: IO14 -> R1 1k -> [C1 100n] -> R2 10k -> [C2 10n] = TP_LA -> R3 220k -> LA -> 200 ohm -> GND
    Canal LL: IO13 -> R1 1k -> [C1 100n] -> R2 10k -> [C2 10n] = TP_LL -> R3 220k -> LL -> 200 ohm -> GND
    RA = N = GND (referencia)

  Derivaciones (las arma el lector restando bornes):
    DI   = LA - RA = V_LA              (canal 1)
    DII  = LL - RA = V_LL              (canal 2)
    DIII = LL - LA = V_LL - V_LA       (aparece sola: DII = DI + DIII)

  Pines ESP32-CAM:
    IO14 -> canal LA (DI)        IO13 -> canal LL (DII)
    IO15 -> botón MODO (a GND)   IO2  -> botón AMP (a GND)   IO12 -> botón RUN (a GND)
    LED de latido: el LED rojo de la placa (GPIO 33, se enciende en LOW)
    No usar IO4 (flash de la cámara) ni IO16 (PSRAM). No poner tarjeta microSD:
    IO2, IO12, IO13, IO14 e IO15 también son las líneas de la ranura SD.

  Placa: "AI Thinker ESP32-CAM" (paquete esp32 3.x). Para subir: base ESP32-CAM-MB.
*/

const int PIN_LA     = 14;
const int PIN_LL     = 13;
const int BTN_MODO   = 15;
const int BTN_AMP    = 2;
const int BTN_RUN    = 12;
const int LED_PLACA  = 33;         // LED rojo integrado (activo en LOW)
const int FLASH_CAM  = 4;          // flash de la cámara: apagado

// ---- PWM: se usa la primera frecuencia que la placa acepte ----
const uint32_t OPC_FREQ[] = {62500, 39062, 19531};
uint32_t PWM_FREQ = 0;
const uint8_t  PWM_BITS = 10;
const int      PWM_MAX  = (1 << PWM_BITS) - 1;

// ---- Circuito (igual para los dos canales) ----
const float VDD        = 3.3f;
const float G_FILTRO   = 0.952f;   // 220,2k / (11k + 220,2k)
// Atenuación TP / borne = (R3 + R4) / R4. Con los valores nominales: (220k + 200) / 200 = 1101.
// CALIBRACIÓN: mide R3 y R4 de cada canal con el multímetro y pon aquí el valor real de cada uno.
// Así DIII (que es la resta de los dos canales) queda dentro de la tolerancia.
const float ATEN_LA    = 1101.0f;  // canal LA (IO14)
const float ATEN_LL    = 1101.0f;  // canal LL (IO13)
const float TP_BASE    = 0.80f;    // línea base en cada TP (V)

// ---- Forma de cada derivación (alturas en mV, con escala 1) ----
// Cada canal tiene una forma distinta, para que la resta (DIII = DII - DI) dé una
// derivación con forma propia: R chica, S marcada y T invertida.
//                       P      Q       R      S       T
const float AMP_DI[]  = {0.10f, -0.03f, 0.80f, -0.05f, 0.35f};   // canal LA
const float AMP_DII[] = {0.15f, -0.12f, 1.00f, -0.30f, 0.20f};   // canal LL
//  => DIII (automática): P 0,05 · Q -0,09 · R 0,20 · S -0,25 · T -0,15 mV

const int   BPM[]   = {40, 60, 120, 180};
const int   N_MODOS = 5;           // 4 ritmos + calibración
int   modo      = 1;               // arranca en 60 lpm
float escala    = 1.0f;            // botón AMP: x1 / x2
bool  corriendo = true;

const int MAX_RR = 1500;
uint16_t tablaLA[MAX_RR], tablaLL[MAX_RR];
int largo = 1000, idxR = 400;

hw_timer_t* timer = nullptr;
volatile uint32_t ticks = 0;       // lo suma el timer cada 1 ms
uint32_t hechos = 0;               // milisegundos ya enviados a los PWM
void ARDUINO_ISR_ATTR onTimer() { ticks = ticks + 1; }

uint16_t mV_a_duty(float mv, float aten) {
  float tp   = TP_BASE + mv * aten / 1000.0f;
  float duty = tp / (G_FILTRO * VDD);
  return (uint16_t)constrain((int)roundf(duty * PWM_MAX), 0, PWM_MAX);
}

float g(float t, float c, float w, float a) { float x = (t - c) / w; return a * expf(-0.5f * x * x); }

// Arma un latido para una derivación con sus 5 alturas
void armar(uint16_t* tabla, const float* A, int RR, float aten) {
  float k = sqrtf(RR / 1000.0f), R = 0.40f * RR;
  static float e[MAX_RR];
  for (int i = 0; i < RR; i++) {
    float t = i;
    e[i] = g(t, R - 190 * k, 22 * k, A[0]) + g(t, R - 25, 8, A[1]) + g(t, R, 9, A[2])
         + g(t, R + 25, 8, A[3]) + g(t, R + 240 * k, 40 * k, A[4]);
  }
  float e0 = e[0], eN = e[RR - 1];
  for (int i = 0; i < RR; i++)
    tabla[i] = mV_a_duty((e[i] - (e0 + (eN - e0) * i / (float)(RR - 1))) * escala, aten);
}

void construirTablas() {
  if (modo == 4) {                                 // calibración: 1 mV en DI y DII, 1 Hz
    largo = 1000; idxR = 0;
    for (int i = 0; i < largo; i++) {
      tablaLA[i] = mV_a_duty(i < 500 ? 1.0f : 0.0f, ATEN_LA);
      tablaLL[i] = mV_a_duty(i < 500 ? 1.0f : 0.0f, ATEN_LL);
    }
    Serial.println("Modo: CALIBRACION 1 Hz / 1 mV (DI y DII; DIII = 0)");
    hechos = ticks;
    return;
  }
  int RR = 60000 / BPM[modo];
  largo = RR; idxR = (int)(0.40f * RR);
  armar(tablaLA, AMP_DI,  RR, ATEN_LA);
  armar(tablaLL, AMP_DII, RR, ATEN_LL);
  Serial.printf("Modo: %d lpm | R: DI %.2f, DII %.2f, DIII %.2f mV\n", BPM[modo],
                AMP_DI[2] * escala, AMP_DII[2] * escala, (AMP_DII[2] - AMP_DI[2]) * escala);
  hechos = ticks;                  // descarta el tiempo usado en recalcular: sin ráfagas al cambiar de modo
}

bool presionado(int pin, bool& previo, uint32_t& t0) {
  bool ahora = digitalRead(pin) == LOW, flanco = false;
  if (ahora && !previo && millis() - t0 > 200) { flanco = true; t0 = millis(); }
  previo = ahora;
  return flanco;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(FLASH_CAM, OUTPUT); digitalWrite(FLASH_CAM, LOW);
  pinMode(LED_PLACA, OUTPUT); digitalWrite(LED_PLACA, HIGH);   // apagado
  pinMode(BTN_MODO, INPUT_PULLUP);
  pinMode(BTN_AMP,  INPUT_PULLUP);
  pinMode(BTN_RUN,  INPUT_PULLUP);

  for (int i = 0; i < 3 && PWM_FREQ == 0; i++) {
    if (ledcAttach(PIN_LA, OPC_FREQ[i], PWM_BITS)) {
      if (ledcAttach(PIN_LL, OPC_FREQ[i], PWM_BITS)) PWM_FREQ = OPC_FREQ[i];
      else ledcDetach(PIN_LA);
    }
  }
  if (PWM_FREQ == 0) { Serial.println("ERROR: no se pudo configurar el PWM"); while (true) delay(1000); }
  Serial.printf("PWM OK: %u Hz, %u bits, 2 canales (IO14 = LA, IO13 = LL)\n", (unsigned)PWM_FREQ, (unsigned)PWM_BITS);

  construirTablas();
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000, true, 0);                // cada 1 ms
  Serial.println("Simulador listo.");
}

void loop() {
  static bool pM = false, pA = false, pR = false;
  static uint32_t tM = 0, tA = 0, tR = 0;
  static int idx = 0;

  if (presionado(BTN_MODO, pM, tM)) { modo = (modo + 1) % N_MODOS; idx = 0; construirTablas(); }
  if (presionado(BTN_AMP,  pA, tA)) { escala = (escala == 1.0f) ? 2.0f : 1.0f; construirTablas(); }
  if (presionado(BTN_RUN,  pR, tR)) { corriendo = !corriendo; Serial.println(corriendo ? "RUN" : "STOP"); }

  while (hechos != ticks) {                        // una vez por cada milisegundo
    hechos++;
    if (corriendo) {
      ledcWrite(PIN_LA, tablaLA[idx]);
      ledcWrite(PIN_LL, tablaLL[idx]);
      int d = idx - idxR;
      digitalWrite(LED_PLACA, (d >= 0 && d < 60) ? LOW : HIGH);
      idx = (idx + 1) % largo;
    } else {
      ledcWrite(PIN_LA, mV_a_duty(0, ATEN_LA));
      ledcWrite(PIN_LL, mV_a_duty(0, ATEN_LL));
      digitalWrite(LED_PLACA, HIGH);
    }
  }
}
