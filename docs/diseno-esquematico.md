# Diseño del esquemático — Simulador ECG (Parte II)

Plano completo del circuito para construir el esquemático en KiCad. Todos los
componentes son **THT**. El medidor de carga (SPBKBS-10) queda **fuera por ahora**.

> **Seguridad (invariante de diseño):** la atenuación es **pasiva y fija**. Aunque el
> firmware falle y saque PWM al 100 %, la salida en los bornes se mantiene en el orden
> de **mV** (≈2 mV máx en DII). El equipo es una fuente de prueba; **no se conecta a
> personas**.

---

## 1. Diagrama de bloques

```
         USB-C (carga)                         USB-C (programación/alimentación)
              │                                             │
   LiPo 1S ─► TP4056(M1) ─► SW1 ─► MT3608(M2) ─► D1 ─► +5V ─┴─► DevKit 5V ─► (LDO) ─► 3V3
   3.7V       carga/BMS    on/off  boost 5.3V   Schottky  │
                                                 C3,C4 ────┘ (desacople cerca del 5V)

   ESP32-S3 GPIO4 (PWM) ─► R1,C1 ─► R2,C2 ─► SIG (volts) ─┬─► TP1 / BNC (osciloscopio)
                             (filtro RC 2 etapas)         │
                                                          └─► R3 (atenuador) ─► red de
                                                              derivaciones ─► bornes
   ESP32-S3 GPIO5 ─► R7 ─► D3 (LED Bluetooth)
   +5V ─► R6 ─► D2 (LED encendido)
```

---

## 2. Lista de componentes (BOM del esquemático)

| Ref. | Valor / modelo | Descripción | Tipo |
|---|---|---|---|
| U1 | ESP32-S3-DevKitC-1-N16R8V | MCU (por headers hembra) | Módulo |
| BT1 | LiPo 1S 3.7 V | Batería (conector JST-PH 2 pines) | Conector |
| M1 | TP4056 USB-C 1A | Cargador + protección BMS | Módulo |
| SW1 | SPST | Interruptor on/off | THT |
| M2 | MT3608 | Boost DC-DC (ajustar a ~5.3 V) | Módulo |
| D1 | 1N5819 | Diodo Schottky (protección) | THT |
| C3 | 100 µF | Desacople 5 V (reservorio) | Electrolítico |
| C4 | 100 nF | Desacople 5 V (alta frecuencia) | Cerámico |
| R1 | 1 kΩ | Filtro RC etapa 1 | THT |
| C1 | 100 nF | Filtro RC etapa 1 | Cerámico |
| R2 | 10 kΩ | Filtro RC etapa 2 | THT |
| C2 | 10 nF | Filtro RC etapa 2 | Cerámico |
| R3 | 330 kΩ | Atenuador (volts → mV) | THT |
| R4 | 100 Ω | Red derivaciones (LL–LA) | THT |
| R5 | 100 Ω | Red derivaciones (LA–RA) | THT |
| TP1 | — | Punto de prueba (volts) | Testpoint |
| J_BNC | BNC CN381 | Salida a osciloscopio | THT |
| J_RA, J_LA, J_LL, J_RL | Banana 4 mm hembra | Bornes de electrodos | THT |
| D2 | LED verde | Indicador de encendido | THT |
| R6 | 330 Ω | Limitador de D2 | THT |
| D3 | LED azul | Indicador de Bluetooth | THT |
| R7 | 330 Ω | Limitador de D3 | THT |

---

## 3. Netlist (conexiones nodo por nodo)

### Alimentación
- **VBAT:** BT1(+) ↔ M1.B+
- **GND:** BT1(–) ↔ M1.B– ↔ M1.OUT– ↔ M2.VIN– ↔ M2.VOUT– ↔ C3(–) ↔ C4 ↔ U1.GND ↔ (referencias de LEDs y de la red de electrodos)
- **VSW:** M1.OUT+ ↔ SW1.1 ; SW1.2 ↔ M2.VIN+
- **+5V:** M2.VOUT+ ↔ D1.ánodo ; D1.cátodo ↔ +5V ↔ C3(+) ↔ C4 ↔ U1.5V ↔ R6
- **3V3:** U1.3V3 (disponible; sin carga externa por ahora)

### LEDs de estado
- **D2 (encendido):** +5V → R6(330 Ω) → D2.ánodo ; D2.cátodo → GND
- **D3 (Bluetooth):** U1.GPIO5 → R7(330 Ω) → D3.ánodo ; D3.cátodo → GND

### Generación de señal (PWM → filtro → volts)
- U1.GPIO4 → R1 → **nodo A**
- **nodo A** → C1 → GND
- **nodo A** → R2 → **SIG** (señal en volts)
- **SIG** → C2 → GND
- **SIG** → TP1
- **SIG** → J_BNC.centro ; J_BNC.malla → GND

### Atenuador + red de derivaciones (volts → mV)
- **SIG** → R3(330 kΩ) → **LL**
- **LL** → R4(100 Ω) → **LA**
- **LA** → R5(100 Ω) → GND (= RA)
- **LL** → J_LL
- **LA** → J_LA
- J_RA → GND
- J_RL → GND

**Resultado (a fondo de escala, SIG ≈ 3.3 V):**
- VLL ≈ SIG · 200/(330k+200) ≈ **2.0 mV**
- DII = VLL − VRA ≈ **2 mV**
- DI = VLA − VRA ≈ **1 mV**
- DIII = VLL − VLA ≈ **1 mV**  → cumple DII = DI + DIII ✓

La amplitud fina (p. ej. niveles ~1 y ~2 mV) se ajusta **por software** variando la
amplitud del PWM, no por hardware.

---

## 4. Asignación de GPIOs (ESP32-S3-DevKitC-1-N16R8V)

| Función | GPIO | Motivo |
|---|---|---|
| Salida PWM de señal | **GPIO4** | LEDC/PWM, libre, no es strapping |
| LED Bluetooth | **GPIO5** | Libre, no es strapping |
| LED encendido | — | Cableado al riel +5V (sin GPIO) |

**Pines evitados:** strapping (0, 3, 45, 46), USB (19, 20), y los de la **PSRAM octal
"R8"** (33–37). UART por defecto (43/44) se deja para la consola serie.

---

## 5. Notas de implementación

- **MT3608 a ~5.3 V:** compensa la caída del Schottky D1 (~0.3–0.4 V) para que al pin
  5V del DevKit lleguen ~5.0 V. Ajustar con el potenciómetro del módulo y verificar
  con multímetro antes de conectar el DevKit.
- **C3/C4 cerca del pin 5V** del DevKit (no pegados al MT3608) para absorber los picos
  de corriente del Bluetooth y evitar reinicios.
- **Dos USB-C separados y es correcto:** el del M1 (TP4056) solo carga la batería; el
  del DevKit programa/alimenta el ESP32. D1 impide que el 5V del DevKit retroalimente
  al MT3608.
- **Bornes banana 4 mm:** si los electrodos del lector (Proyecto 4) terminan en broche
  (snap), habría que usar bornes tipo snap; confirmar con qué terminan los cables.
- **Reglas CNC del lab:** pista mín 0,8 mm, separación mín 0,8 mm, 4 agujeros 3,2 mm,
  ruteo prioritario en B.Cu.

---

## 6. Pendientes para KiCad

1. Importar la librería **SnapEDA** de U1 (símbolo + footprint + STEP) a
   `electronica/libraries/` y enlazar la tabla de librerías del proyecto.
2. Definir/elegir footprints THT de módulos (M1, M2) y conectores (banana, BNC, JST).
3. Dibujar el esquemático v2 con esta netlist (reemplazar `power:+1V0` por la fuente
   real: GPIO4 del ESP32-S3).
4. (Más adelante) medidor de carga SPBKBS-10: símbolo + footprint propios (hay STEP).
