/*
  Simulador ECG — versión para TINKERCAD (Arduino Uno)
  -----------------------------------------------------
  Tinkercad no tiene ESP32, así que esta versión imita el diseño con un Uno:
  - PWM en el pin 9 a 62,5 kHz y 8 bits (Timer1 reconfigurado).
    Con analogWrite() normal serían solo 490 Hz y la señal saldría con mucho rizado.
  - Lógica de 5 V en vez de 3,3 V (ya considerado en VDD).
  - Una muestra nueva cada 2 ms (500 Hz) para que la tabla quepa en la RAM del Uno.

  Circuito: pin 9 -> R1 1k -> [C1 100n a GND] -> R2 10k -> [C2 10n a GND] = TP1
            TP1 -> R3 100k -> LL -> R4 100 -> LA -> R5 100 -> RA = GND
  Medir: osciloscopio en TP1 (~0,7 a 1,3 V). Multímetro DC en LL y LA (mV).
*/

const int   PIN_PWM   = 9;        // OC1A
const int   BPM       = 60;       // cambiar a 40, 60, 120 o 180
const float AMP_mV    = 1.0;      // onda R en DII: 1.0 o 2.0
const bool  DETENIDO  = false;    // true = línea base fija (para medir con multímetro)

const float VDD       = 5.0;      // nivel alto del pin en el Uno
const float G_FILTRO  = 0.901;    // ganancia DC hasta TP1
const float ATEN      = 501.0;    // TP1 / DII
const float TP1_BASE  = 0.80;     // volts en TP1 para la línea base

const int   FS        = 500;      // muestras por segundo
const int   MAX_N     = 750;      // 40 lpm -> 1,5 s -> 750 muestras
uint8_t tabla[MAX_N];
int largo;

uint8_t mV_a_duty(float dII_mV) {
  float tp1 = TP1_BASE + dII_mV * ATEN / 1000.0;
  int c = (int)(tp1 / (G_FILTRO * VDD) * 255.0 + 0.5);
  return (uint8_t)constrain(c, 0, 255);
}

float g(float t, float c, float w, float a) { float x = (t - c) / w; return a * exp(-0.5 * x * x); }

void construirTabla() {
  int RR = 60000 / BPM;                    // ms
  largo = RR * FS / 1000;                  // muestras
  float k = sqrt(RR / 1000.0), R = 0.40 * RR;
  float e0 = 0, eN = 0;
  for (int pasada = 0; pasada < 2; pasada++) {
    for (int i = 0; i < largo; i++) {
      float t = i * 1000.0 / FS;           // ms
      float e = g(t, R - 190 * k, 22 * k, 0.15) + g(t, R - 25, 8, -0.12) + g(t, R, 9, 1.0)
              + g(t, R + 25, 8, -0.25) + g(t, R + 240 * k, 40 * k, 0.30);
      if (pasada == 0) { if (i == 0) e0 = e; if (i == largo - 1) eN = e; }
      else {
        float rampa = e0 + (eN - e0) * i / (float)(largo - 1);
        tabla[i] = mV_a_duty((e - rampa) * AMP_mV);
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_PWM, OUTPUT);
  // Timer1: Fast PWM 8 bits, sin prescaler -> 16 MHz / 256 = 62,5 kHz en el pin 9
  TCCR1A = _BV(COM1A1) | _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  construirTabla();
  Serial.print("BPM="); Serial.print(BPM);
  Serial.print("  duty base="); Serial.print(mV_a_duty(0));
  Serial.print("/255  duty pico R="); Serial.println(mV_a_duty(AMP_mV));
}

void loop() {
  static unsigned long t0 = micros();
  static int idx = 0;
  if (micros() - t0 >= 1000000UL / FS) {
    t0 += 1000000UL / FS;
    OCR1A = DETENIDO ? mV_a_duty(0) : tabla[idx];
    idx = (idx + 1) % largo;
  }
}
