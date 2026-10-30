/*
  Simulador de ECG de 3 derivaciones — ESP32-S3 + PWM
  ----------------------------------------------------
  Cadena: GPIO (PWM 10 bits, 78 kHz) -> filtro RC 2 etapas (TP1, volts)
          -> R3 100k / R4 100 / R5 100 -> bornes LL, LA, RA, N  (milivolts)

  Requiere: placa "esp32 by Espressif Systems" versión 3.x en el Arduino IDE.

  Controles:
    BTN_MODO  : cicla 40 -> 60 -> 120 -> 180 lpm -> calibración (1 Hz, 1 mV)
    BTN_AMP   : alterna onda R de 1 mV / 2 mV en DII
    BTN_RUN   : inicia / detiene la salida (detenido = línea base)
    LED_LATIDO: se enciende en cada onda R
*/

// ---------------- Pines (ajustar según la placa; evitar 0, 3, 45, 46) ----------------
const int PIN_PWM    = 4;
const int LED_LATIDO = 5;
const int BTN_MODO   = 6;
const int BTN_AMP    = 7;
const int BTN_RUN    = 15;

// ---------------- PWM: f_PWM x 2^bits = 80 MHz ----------------
const uint32_t PWM_FREQ = 78125;   // Hz (portadora)
const uint8_t  PWM_BITS = 10;      // 0..1023
const int      PWM_MAX  = (1 << PWM_BITS) - 1;

// ---------------- Actualización de la muestra ----------------
const uint32_t FS = 1000;          // Hz: un valor nuevo de la tabla cada 1 ms

// ---------------- Constantes del circuito ----------------
const float VDD        = 3.3f;     // nivel alto del GPIO
const float G_FILTRO   = 0.901f;   // ganancia DC hasta TP1 (carga de R3+R4+R5)
const float ATENUACION = 501.0f;   // TP1 / DII  = (R3+R4+R5)/(R4+R5)
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
volatile bool tick = false;
void ARDUINO_ISR_ATTR onTimer() { tick = true; }   // la ISR solo avisa

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
  pinMode(LED_LATIDO, OUTPUT);
  pinMode(BTN_MODO, INPUT_PULLUP);
  pinMode(BTN_AMP,  INPUT_PULLUP);
  pinMode(BTN_RUN,  INPUT_PULLUP);

  ledcAttach(PIN_PWM, PWM_FREQ, PWM_BITS);   // configura la portadora
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

  if (tick) {                                // una vez por milisegundo
    tick = false;
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
