/*
  Réplica del simulador ECG de Riyanto et al. (2025) — Arduino Nano / Uno
  ------------------------------------------------------------------------
  El paper no publica su código. Este código se reconstruyó a partir de su
  esquemático, para replicarlo en Tinkercad (Arduino Uno = mismo chip que el Nano)
  o en un Nano real.

  CIRCUITO (igual al esquema del paper)
    Señal:   D9 -> R5 10k -> [C1 220n a GND] -> R6 10k -> [C2 220n a GND] = TP1
             TP1 -> R7 330k -> nodo X
             nodo X -> R8 100k -> LL (con R10 220 a GND)
             nodo X -> R9 100k -> LA (con R11 220 a GND)
             RA = GND
             D10 -> R12 22k -> LL   (función no explicada en el paper: aquí queda en 0 V)
    Modos:   SW1 -> D2, SW2 -> D3, SW3 -> D4  (cada uno con 10k; en HIGH = activado)
    LED:     D5 -> R1 1k -> LED -> GND        (parpadea con cada onda R)

  MODOS (tabla del paper)
    Ningún switch: 80 lpm (Modo I)   | SW1: 40 lpm (Modo II)
    SW2: 120 lpm (Modo III)          | SW3: 160 lpm (Modo IV)

  QUÉ MEDIR
    TP1 (osciloscopio, 1 V/div, 200 ms/div): ECG entre ~0,1 y ~4,5 V (base en 1,0 V).
    LL y LA: ~1 mV de onda R (divisor ÷3456). DII = LL - RA, DI = LA - RA.

  PWM_RAPIDO = false -> 490 Hz, como un analogWrite() normal (lo más fiel al paper).
  PWM_RAPIDO = true  -> 31,4 kHz en D9/D10 (Timer1 sin divisor): mucho menos rizado.
*/

const bool PWM_RAPIDO = false;

const int PIN_PWM = 9;    // salida de la señal (OC1A, Timer1)
const int PIN_D10 = 10;   // va a LL por R12 22k; en 0 V no altera la señal
const int PIN_SW1 = 2;
const int PIN_SW2 = 3;
const int PIN_SW3 = 4;
const int PIN_LED = 5;

// ---- Circuito de Riyanto ----
const float VDD        = 5.0;     // el Nano trabaja a 5 V
const float G_FILTRO   = 0.950;   // 380,1k / (10k + 10k + 380,1k): carga de R7 + ramas
const float ATENUACION = 3456.0;  // TP1 / LL = (380,1k / 50,1k) * (100,2k / 220)
const float TP1_BASE   = 1.00;    // línea base en TP1 (V): con ÷3456 la S baja 0,86 V, necesita este margen
const float AMP_mV     = 1.0;     // onda R en LL / LA

// ---- Latido: una muestra cada 2 ms (500 Hz) para que la tabla quepa en la RAM ----
const int  FS       = 500;
const int  MAX_N    = 750;        // 40 lpm -> 1,5 s -> 750 muestras
uint8_t    tabla[MAX_N];
int        largo    = 375;
int        idxR     = 0;
int        bpmActual = 0;

uint8_t mV_a_duty(float mv) {
  float tp1 = TP1_BASE + mv * ATENUACION / 1000.0;
  int c = (int)(tp1 / (G_FILTRO * VDD) * 255.0 + 0.5);
  return (uint8_t)constrain(c, 0, 255);
}

float g(float t, float c, float w, float a) { float x = (t - c) / w; return a * exp(-0.5 * x * x); }

void construirTabla(int bpm) {
  int RR = 60000 / bpm;                 // ms
  largo  = (long)RR * FS / 1000;        // muestras
  float k = sqrt(RR / 1000.0), R = 0.40 * RR;
  idxR = (int)(R * FS / 1000);
  float e0 = 0, eN = 0;
  for (int pasada = 0; pasada < 2; pasada++) {
    for (int i = 0; i < largo; i++) {
      float t = i * 1000.0 / FS;
      float e = g(t, R - 190 * k, 22 * k, 0.15) + g(t, R - 25, 8, -0.12) + g(t, R, 9, 1.0)
              + g(t, R + 25, 8, -0.25) + g(t, R + 240 * k, 40 * k, 0.30);
      if (pasada == 0) { if (i == 0) e0 = e; if (i == largo - 1) eN = e; }
      else tabla[i] = mV_a_duty((e - (e0 + (eN - e0) * i / (float)(largo - 1))) * AMP_mV);
    }
  }
  Serial.print("Modo: "); Serial.print(bpm); Serial.print(" lpm | duty base = ");
  Serial.print(mV_a_duty(0)); Serial.print("/255 | pico R = "); Serial.println(mV_a_duty(AMP_mV));
}

int leerModo() {
  if (digitalRead(PIN_SW1) == HIGH) return 40;
  if (digitalRead(PIN_SW2) == HIGH) return 120;
  if (digitalRead(PIN_SW3) == HIGH) return 160;
  return 80;
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_PWM, OUTPUT);
  pinMode(PIN_D10, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SW1, INPUT);              // el circuito ya trae sus resistencias de 10k
  pinMode(PIN_SW2, INPUT);
  pinMode(PIN_SW3, INPUT);
  digitalWrite(PIN_D10, LOW);

  if (PWM_RAPIDO) {
    TCCR1B = (TCCR1B & 0b11111000) | 0x01;   // Timer1 sin divisor: 31,4 kHz en D9/D10
  }
  Serial.println(PWM_RAPIDO ? "PWM: 31,4 kHz" : "PWM: 490 Hz (analogWrite normal)");

  bpmActual = leerModo();
  construirTabla(bpmActual);
}

void loop() {
  static unsigned long t0 = micros();
  static int idx = 0;

  int bpm = leerModo();                  // cambio de modo con los switches
  if (bpm != bpmActual) { bpmActual = bpm; construirTabla(bpm); idx = 0; t0 = micros(); }

  if (micros() - t0 >= 1000000UL / FS) { // cada 2 ms
    t0 += 1000000UL / FS;
    analogWrite(PIN_PWM, tabla[idx]);
    int d = idx - idxR;
    digitalWrite(PIN_LED, (d >= 0 && d < 30) ? HIGH : LOW);   // LED ~60 ms en cada R
    idx = (idx + 1) % largo;
  }
}
