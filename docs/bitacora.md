# Bitácora del proyecto

Registro cronológico de avances, decisiones y problemas.

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
