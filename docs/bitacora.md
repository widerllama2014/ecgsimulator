# Bitácora del proyecto

Registro cronológico de avances, decisiones y problemas.

## 2026-10-27 — Esquemático reorganizado en 3 bloques con cableado real

- Se reorganizó el esquemático en **tres bloques** con marco y título, cableado con
  **cables reales** (no solo net labels): **ALIMENTACIÓN**, **CONEXIONES ESP32-S3** y
  **CIRCUITO DE SEÑAL (ECG)**, al estilo del primer prototipo.
- GND y +5V con símbolos de power; `PWM` como etiqueta que cruza del bloque ESP32 al de
  señal. Negativo de la batería referenciado a GND.
- Revalidado con `kicad-cli`: **ERC 0 violaciones** y **netlist verificada** (sin
  cortocircuitos; todas las redes coinciden con el diseño). Generado con
  `scratchpad/gen2.py` (incluye la transformación de rotación calibrada).

## 2026-10-27 — Esquemático KiCad completo y validado (ERC 0 violaciones)

- Se cerró el **esquemático completo** en `electronica/kicad/Proyecto de prueba.kicad_sch`
  con todos los bloques: ESP32-S3, alimentación (BT1, M1 TP4056, SW1, M2 MT3608, D1,
  C3/C4), señal (R1/C1/R2/C2, TP1, BNC, R3, R4/R5, bornes LL/LA/RA/RL) y LEDs de
  estado (D2+R6 encendido, D3+R7 Bluetooth).
- **Conectividad por net labels**; validado con `kicad-cli sch erc` → **0 violaciones**.
  Netlist verificada: +5V, V5RAW, VSW, VBAT, GND, PWM, NODEA, SIG, LLN, LAN, PLED, BLED
  coinciden con el diseño. Pines no usados del ESP32 marcados como no-conectados.
- Símbolos de los módulos TP4056/MT3608 ajustados a pines `passive`; `PWR_FLAG` en +5V y
  GND. Librerías registradas (sym: MT3608_DC; fp: ESP32, MT3608_DC, TP4056_modulo).
- Respaldo del esquemático anterior: `Proyecto de prueba.kicad_sch.bak`.
- **Pendiente:** asignar footprints THT a pasivos/conectores; verificar medidas del
  footprint TP4056; layout de PCB (reglas CNC); el medidor SPBKBS-10 sigue excluido.

## 2026-10-27 — Diseño completo del circuito (netlist para KiCad)

- Se cerró el **diseño completo del esquemático** → `docs/diseno-esquematico.md`
  (BOM, netlist nodo por nodo, asignación de GPIOs y notas).
- **Decisiones finales:** se incluye **diodo Schottky D1 (1N5819)**; bornes de electrodo
  = **banana 4 mm hembra** (conexión directa del cable); GPIOs asignados
  (PWM=GPIO4, LED Bluetooth=GPIO5; LED encendido cableado al +5V).
- **Atenuador:** R3 = 330 kΩ con red R4/R5 (100 Ω) → DII≈2 mV, DI≈DIII≈1 mV a fondo de
  escala. Atenuación pasiva fija = **garantía de salida en mV aunque falle el firmware**.
- El medidor de carga (SPBKBS-10) queda **excluido por ahora** del esquemático.

## 2026-10-27 — Selección de componentes y opciones de compra

- **Librería del MCU:** se adopta la de SnapEDA para **ESP32-S3-DevKitC-1-N16R8V**
  (símbolo + footprint + STEP) → `electronica/libraries/`.
- **Alimentación cerrada:** se confirman **TP4056 (USB-C 1A)** + **boost MT3608 a 5 V**.
  El boost sube los 3,7 V de la LiPo 1S a 5 V (no se necesita batería de más voltaje).
- **Conector a osciloscopio:** BNC macho RG58 (CN381).
- **Medidor de nivel de carga 1S–8S** reemplaza al LED indicador de batería (D4/R8
  eliminados). *Pendiente: generar símbolo + footprint propios; hay STEP disponible.*
- **Se mantiene un LED de encendido (on/off)** y el LED de Bluetooth (GPIO).
- **Condensadores** C3 (100 µF) + C4 (100 nF) a la salida de 5 V del MT3608, cerca del
  pin 5 V del DevKit, para absorber los picos de corriente del Bluetooth.
- Opciones de compra y alternativas documentadas en `docs/compras.md`.

## 2026-10-27 — Reorganización del repositorio y definición de alcance (Parte II)

- Se reestructuró el repositorio según la estructura exigida por la rúbrica de la
  Parte II (`docs/`, `electronica/{kicad,libraries,outputs}/`, `firmware/`, `software/`).
- Se movió el proyecto de KiCad a `electronica/kicad/` y la biblioteca propia a
  `electronica/libraries/`. El informe de la Parte I quedó en `docs/informes/`.
- **Decisiones de diseño confirmadas:**
  - Microcontrolador: **ESP32-S3-DevKitC-1** (montado por headers).
  - **4 electrodos (RA/LA/LL/RL-N) → 3 derivaciones** bipolares (DI, DII, DIII).
  - El proyecto se divide en 3: **PCB física**, **software** (PC/teléfono) y
    **comunicación por Bluetooth (BLE)**.
  - La **interfaz** (selección de modo, amplitud, señal e interferencias) se realiza
    en **software**; en la placa quedan **LEDs de estado THT** (encendido, Bluetooth,
    batería).
  - Conectores en placa (no son salidas de ECG): **USB**, **cargador de batería**,
    **salida a osciloscopio (BNC)**.
  - Alimentación propuesta: **LiPo + TP4056** (carga) **+ boost a 5 V** → pin 5 V del
    DevKit. *(A confirmar.)*
  - Todos los componentes **THT**.
- **Pendiente:** cerrar detalle de batería/carga; integrar ESP32-S3, alimentación,
  LEDs y conectores en el esquemático; verificar bibliotecas; layout CNC.

<!-- Nuevas entradas arriba de esta línea, de más reciente a más antigua. -->
