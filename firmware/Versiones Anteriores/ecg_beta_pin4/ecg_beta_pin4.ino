/*
  Simulador ECG — VERSIÓN BETA (solo salida por el GPIO 4)
  ---------------------------------------------------------
  - Sin botones ni LED: al encender, el ESP32-S3 manda el ECG sin parar.
  - 60 lpm, onda R de 1 mV en DII.
  - PWM de 10 bits a ~39 kHz en el GPIO 4 -> filtro RC -> TP1 -> divisor (R3 = 220k).

  Conexión mínima:  GPIO 4 -> R1      GND -> riel negro
  Esperado en TP1 (osciloscopio): ECG entre ~0,55 y 1,9 V, un latido por segundo.

  Placa en el Arduino IDE: "ESP32S3 Dev Module" (paquete esp32 versión 3.x)
*/

const int PIN_PWM = 4;

// El código prueba estas combinaciones en orden y usa la primera que el ESP32 acepte.
// En nuestra ESP32-S3 el reloj del PWM es de 40 MHz: queda en 39 062 Hz (39 062 x 1024 = 40 MHz).
const uint32_t OPC_FREQ[] = {62500, 39062, 19531};
const uint8_t  OPC_BITS[] = {10,    10,    10};
uint32_t PWM_FREQ = 0;
uint8_t  PWM_BITS = 10;
int      PWM_MAX  = 1023;

const int   BPM        = 60;       // frecuencia cardíaca fija
const float AMP_mV     = 1.0f;     // onda R en DII

// Constantes del circuito (R1=1k, C1=100n, R2=10k, C2=10n, R3=220k, R4=R5=100)
const float VDD        = 3.3f;
const float G_FILTRO   = 0.952f;   // ganancia DC hasta TP1: 220,2k / (11k + 220,2k)
const float ATENUACION = 1101.0f;  // TP1 / DII = (R3+R4+R5)/(R4+R5) = 220,2k / 200
const float TP1_BASE   = 0.80f;    // línea base en TP1 (V)

const int RR = 60000 / BPM;        // 1000 ms = 1000 muestras (1 kHz)
uint16_t tabla[1500];

hw_timer_t* timer = nullptr;
volatile uint32_t ticks = 0;       // contador: no se pierden milisegundos
void ARDUINO_ISR_ATTR onTimer() { ticks++; }

uint16_t mV_a_duty(float dII_mV) {
  float tp1  = TP1_BASE + dII_mV * ATENUACION / 1000.0f;
  float duty = tp1 / (G_FILTRO * VDD);
  return (uint16_t)constrain((int)roundf(duty * PWM_MAX), 0, PWM_MAX);
}

float g(float t, float c, float w, float a) { float x = (t - c) / w; return a * expf(-0.5f * x * x); }

void construirTabla() {
  float k = sqrtf(RR / 1000.0f), R = 0.40f * RR;
  static float e[1500];
  for (int i = 0; i < RR; i++) {
    float t = i;
    e[i] = g(t, R - 190 * k, 22 * k, 0.15f) + g(t, R - 25, 8, -0.12f) + g(t, R, 9, 1.00f)
         + g(t, R + 25, 8, -0.25f) + g(t, R + 240 * k, 40 * k, 0.30f);
  }
  float e0 = e[0], eN = e[RR - 1];
  for (int i = 0; i < RR; i++) {
    float rampa = e0 + (eN - e0) * i / (float)(RR - 1);
    tabla[i] = mV_a_duty((e[i] - rampa) * AMP_mV);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  for (int i = 0; i < 3 && PWM_FREQ == 0; i++) {
    if (ledcAttach(PIN_PWM, OPC_FREQ[i], OPC_BITS[i])) {
      PWM_FREQ = OPC_FREQ[i]; PWM_BITS = OPC_BITS[i]; PWM_MAX = (1 << PWM_BITS) - 1;
    }
  }
  if (PWM_FREQ == 0) {
    Serial.println("ERROR: no se pudo configurar el PWM en el GPIO 4");
    while (true) delay(1000);
  }
  Serial.printf("PWM OK: %u Hz, %u bits\n", PWM_FREQ, PWM_BITS);
  construirTabla();
  timer = timerBegin(1000000);              // 1 MHz
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000, true, 0);         // cada 1 ms
  Serial.printf("ECG beta: %d lpm, R = %.1f mV, duty base = %d/%d\n",
                BPM, AMP_mV, mV_a_duty(0), PWM_MAX);
}

void loop() {
  static uint32_t hechos = 0;
  static int idx = 0;
  while (hechos != ticks) {                 // procesa cada milisegundo pendiente
    hechos++;
    ledcWrite(PIN_PWM, tabla[idx]);
    idx = (idx + 1) % RR;
  }
}
