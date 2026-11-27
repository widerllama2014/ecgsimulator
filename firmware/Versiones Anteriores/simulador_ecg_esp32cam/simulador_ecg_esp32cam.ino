/*
  Simulador de ECG de 3 derivaciones — ESP32-CAM (AI-Thinker) + PWM — CIRCUITO V2
  -------------------------------------------------------------------------------
  Cadena: GPIO 14 (PWM 10 bits) -> R1 1k -> [C1 100n] -> R2 10k -> [C2 10n] = TP1
          -> R3 220k -> LL -> R4 100 -> LA -> R5 100 -> RA = N = GND

  Placa en el Arduino IDE: "AI Thinker ESP32-CAM" (paquete esp32 versión 3.x)
  Para subir: base ESP32-CAM-MB, o adaptador USB-serie con IO0 unido a GND
  (después de subir, quitar ese puente y presionar RST).

  Pines (los pocos libres de la ESP32-CAM):
    GPIO 14 -> salida PWM (señal ECG) -> R1
    GPIO 13 -> LED de latido (con 330 ohm a GND)
    GPIO 15 -> botón MODO  (a GND)     40 -> 60 -> 120 -> 180 lpm -> calibración
    GPIO 2  -> botón AMP   (a GND)     1 mV / 2 mV en DII
    GPIO 12 -> botón RUN   (a GND)     inicio / detención
    GND     -> riel de tierra
  Los botones son opcionales: sin ellos, arranca en 60 lpm y 1 mV.
  No usar GPIO 4 (flash blanco de la cámara) ni GPIO 16 (memoria PSRAM).
  No mantener presionados los botones de GPIO 2 / 12 al encender.

  Esperado en TP1 (osciloscopio, 0,2 V/div, 200 ms/div): ECG entre ~0,55 y 1,9 V.
*/

// ---------------- Pines (ESP32-CAM) ----------------
const int PIN_PWM    = 14;
const int LED_LATIDO = 13;
const int BTN_MODO   = 15;
const int BTN_AMP    = 2;
const int BTN_RUN    = 12;
const int FLASH_CAM  = 4;          // LED de flash de la cámara: se deja apagado

// ---------------- PWM: f_PWM x 2^bits <= reloj del LEDC ----------------
// Se prueban opciones en orden y se usa la primera que la placa acepte
// (f x 1024 debe caber en el reloj del PWM: 80 MHz en el ESP32 clásico, 40 MHz en algunos S3).
const uint32_t OPC_FREQ[] = {62500, 39062, 19531};
uint32_t PWM_FREQ = 0;
const uint8_t  PWM_BITS = 10;      // 0..1023
const int      PWM_MAX  = (1 << PWM_BITS) - 1;

// ---------------- Actualización de la muestra ----------------
const uint32_t FS = 1000;          // Hz: un valor nuevo de la tabla cada 1 ms

// ---------------- Constantes del circuito ----------------
const float VDD        = 3.3f;     // nivel alto del GPIO
const float G_FILTRO   = 0.952f;   // ganancia DC hasta TP1: 220,2k / (11k + 220,2k)
const float ATENUACION = 1101.0f;  // TP1 / DII = (R3+R4+R5)/(R4+R5) = 220,2k / 200
const float TP1_BASE   = 0.80f;    // V en TP1 para la línea base (deja espacio a Q y S)

// ---------------- Modos ----------------
const int   BPM[]   = {40, 60, 120, 180};
const int   N_MODOS = 5;           // 4 ritmos + calibración
int   modo     = 1;                // arranca en 60 lpm
float ampR_mV  = 1.0f;             // amplitud de la onda R en DII
bool  corriendo = true;

// ---------------- Tabla de un latido ----------------
const int MAX_RR = 1500;           // 40 lpm -> 1500 ms -> 1500 muestras
uint16_t tabla[MAX_RR];
int      largoTabla = 1000;
int      idxR       = 400;         // muestra donde cae la onda R (para el LED)
volatile int idx    = 0;

// ---------------- Timer de hardware ----------------
hw_timer_t* timer = nullptr;
volatile uint32_t ticks = 0;       // contador: no se pierden milisegundos
void ARDUINO_ISR_ATTR onTimer() { ticks++; }

// Convierte un voltaje deseado en DII (mV, sobre la línea base) a cuentas de PWM
uint16_t mV_a_duty(float dII_mV) {
  float tp1  = TP1_BASE + dII_mV * ATENUACION / 1000.0f;   // volts en TP1
  float duty = tp1 / (G_FILTRO * VDD);                     // fracción 0..1
  int   c    = (int)roundf(duty * PWM_MAX);
  return (uint16_t)constrain(c, 0, PWM_MAX);
}

// Gaussiana: cada onda del ECG se modela como una campana
float g(float t, float centro, float ancho, float alto) {
  float x = (t - centro) / ancho;
  return alto * expf(-0.5f * x * x);
}

// Construye la tabla de un latido completo para el modo actual
void construirTabla() {
  if (modo == 4) {                         // calibración: cuadrada 1 Hz, 1 mV
    largoTabla = 1000;
    idxR = 0;
    for (int i = 0; i < largoTabla; i++)
      tabla[i] = mV_a_duty(i < 500 ? 1.0f : 0.0f);
    Serial.println("Modo: CALIBRACION 1 Hz / 1 mV");
    return;
  }

  int   bpm = BPM[modo];
  int   RR  = 60000 / bpm;                 // ms por latido = muestras por latido
  float k   = sqrtf(RR / 1000.0f);         // Bazett: P-R y Q-T se acortan con sqrt(RR)
  float R   = 0.40f * RR;
  largoTabla = RR;
  idxR = (int)R;

  //            centro (ms)        ancho (ms)   alto (x R)
  float P[] = { R - 190 * k,       22 * k,      0.15f };
  float Q[] = { R - 25,            8,          -0.12f };   // el QRS no se estira:
  float Rw[]= { R,                 9,           1.00f };   // su duración casi no cambia
  float S[] = { R + 25,            8,          -0.25f };   // con la frecuencia cardíaca
  float T[] = { R + 240 * k,       40 * k,      0.30f };

  float e0 = 0, eN = 0;
  static float e[MAX_RR];
  for (int i = 0; i < RR; i++) {
    float t = i;
    e[i] = g(t, P[0], P[1], P[2]) + g(t, Q[0], Q[1], Q[2]) + g(t, Rw[0], Rw[1], Rw[2])
         + g(t, S[0], S[1], S[2]) + g(t, T[0], T[1], T[2]);
  }
  e0 = e[0]; eN = e[RR - 1];
  for (int i = 0; i < RR; i++) {
    float rampa = e0 + (eN - e0) * i / (float)(RR - 1);   // empalme suave entre latidos
    tabla[i] = mV_a_duty((e[i] - rampa) * ampR_mV);
  }

  Serial.printf("Modo: %d lpm | RR = %d ms | R = %.1f mV en DII | duty base = %d/%d\n",
                bpm, RR, ampR_mV, mV_a_duty(0), PWM_MAX);
}

// Lectura de pulsador con antirrebote (activo en LOW por INPUT_PULLUP)
bool presionado(int pin, bool& previo, uint32_t& t0) {
  bool ahora = digitalRead(pin) == LOW;
  bool flanco = false;
  if (ahora && !previo && millis() - t0 > 200) { flanco = true; t0 = millis(); }
  previo = ahora;
  return flanco;
}

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_CAM, OUTPUT);
  digitalWrite(FLASH_CAM, LOW);            // flash de la cámara apagado
  pinMode(LED_LATIDO, OUTPUT);
  pinMode(BTN_MODO, INPUT_PULLUP);
  pinMode(BTN_AMP,  INPUT_PULLUP);
  pinMode(BTN_RUN,  INPUT_PULLUP);

  delay(500);
  for (int i = 0; i < 3 && PWM_FREQ == 0; i++)
    if (ledcAttach(PIN_PWM, OPC_FREQ[i], PWM_BITS)) PWM_FREQ = OPC_FREQ[i];
  if (PWM_FREQ == 0) {
    Serial.println("ERROR: no se pudo configurar el PWM en el GPIO 14");
    while (true) delay(1000);
  }
  Serial.printf("PWM OK: %u Hz, %u bits\n", PWM_FREQ, PWM_BITS);
  construirTabla();

  timer = timerBegin(1000000);               // contador a 1 MHz
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000000 / FS, true, 0);  // alarma cada 1 ms, se recarga sola
  Serial.println("Simulador listo.");
}

void loop() {
  static bool pM = false, pA = false, pR = false;
  static uint32_t tM = 0, tA = 0, tR = 0;

  if (presionado(BTN_MODO, pM, tM)) { modo = (modo + 1) % N_MODOS; idx = 0; construirTabla(); }
  if (presionado(BTN_AMP,  pA, tA)) { ampR_mV = (ampR_mV == 1.0f) ? 2.0f : 1.0f; construirTabla(); }
  if (presionado(BTN_RUN,  pR, tR)) { corriendo = !corriendo; Serial.println(corriendo ? "RUN" : "STOP"); }

  static uint32_t hechos = 0;
  while (hechos != ticks) {                  // una vez por cada milisegundo
    hechos++;
    if (corriendo) {
      ledcWrite(PIN_PWM, tabla[idx]);        // nuevo ancho de pulso
      digitalWrite(LED_LATIDO, (idx - idxR >= 0 && idx - idxR < 60) ? HIGH : LOW);
      idx = (idx + 1) % largoTabla;
    } else {
      ledcWrite(PIN_PWM, mV_a_duty(0));      // detenido: línea base
      digitalWrite(LED_LATIDO, LOW);
    }
  }
}
