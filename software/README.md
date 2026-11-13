# Simulador de señales biomédicas (ECG)

Simulador educativo de señales de electrocardiograma (ECG) basado en **ESP32-S3**.
Funciona como **fuente de prueba segura** para equipos de adquisición (lector del
Proyecto 4) y como material de estudio. **No se conecta a personas.**

> Proyecto 5 — *Simulador de señales biomédicas* del banco de proyectos del curso.

## Equipo

| Integrante | Correo | Rol principal |
|---|---|---|
| Benjamín Garrido | b.garridob@udd.cl | Construcción física, esquemático/PCB/BOM, pruebas |
| José Tomás Romero | jt.romeroreyes@udd.cl | Electrónica/investigación, firmware/software, documentación y repositorio |

- **Curso:** Electrónica Biomédica — IBB224S-1, Universidad del Desarrollo (UDD)
- **Docentes:** Alejandro Rebolledo, Wladimir

## Descripción y arquitectura

El sistema se organiza en **tres partes**:

1. **Prototipo físico (esta PCB).** ESP32-S3-DevKitC-1 (montado por headers) + batería.
   Genera la señal por **PWM → filtro RC** y la lleva a nivel fisiológico mediante un
   **divisor/atenuador** y una **red de resistencias** que la reparte en los bornes de
   electrodos **RA / LA / LL / RL(N)**, obteniendo las 3 derivaciones bipolares
   **DI, DII, DIII**. Incluye LEDs de estado (encendido, Bluetooth, batería), punto de
   prueba en volts y 3 conectores: **USB**, **cargador de batería** y **salida a
   osciloscopio (BNC)**. *Los conectores anteriores no son las salidas de ECG.*
2. **Software (teléfono/PC).** Procesamiento, visualización, interfaz y selección/
   modificación de la señal simulada y de las interferencias.
3. **Comunicación.** Enlace ESP32-S3 ↔ equipo externo mediante **Bluetooth (BLE)**.

```
[Software PC/Teléfono] <--BLE--> [ESP32-S3] --PWM--> [Filtro RC] --> TP1 (volts, BNC)
                                                          |
                                                 [Divisor/atenuador]
                                                          |
                                             [Red de resistencias] --> RA / LA / LL / RL(N)
```

## Estructura del repositorio

```
.
├── README.md
├── LICENSE
├── docs/
│   ├── datasheets/      Hojas de datos de los componentes
│   ├── bitacora.md      Registro de avances
│   └── informes/        Informes de entrega (Parte I, II, ...)
├── electronica/
│   ├── kicad/           Proyecto editable de KiCad (.kicad_pro/.sch/.pcb)
│   ├── libraries/       Símbolos, footprints y modelos 3D propios o verificados
│   └── outputs/         Exportables: PDF, Gerbers, drill, BOM, STEP, vistas 3D
├── firmware/            Código del ESP32-S3
└── software/            Aplicación de PC/teléfono
```

## Estado del proyecto

- **Parte I (propuesta y requerimientos):** entregada — ver `docs/informes/`.
- **Parte II (diseño electrónico y PCB):** en desarrollo.
  - [x] Red pasiva (filtro RC + divisor + bornes) iniciada en el esquemático.
  - [ ] Integración del ESP32-S3, alimentación (batería + carga), LEDs y conectores.
  - [ ] Bibliotecas propias verificadas.
  - [ ] Layout según reglas CNC del laboratorio.
  - [ ] Exportables y DRC limpio.

## Reglas de fabricación (CNC del laboratorio)

El diseño debe respetar, de forma obligatoria:

- Ancho mínimo de pista: **0,8 mm**.
- Separación mínima (cobre–cobre): **0,8 mm**.
- Cuatro agujeros de montaje de **3,2 mm** cerca de las esquinas.
- Ruteo priorizado en una sola cara (**B.Cu**); componentes **THT**.

## Licencia

Este proyecto es **hardware abierto** bajo la **CERN Open Hardware Licence
Version 2 – Strongly Reciprocal (CERN-OHL-S v2)** — ver [LICENSE](LICENSE).

Qué permite y qué exige:

- ✅ **Usar, estudiar, modificar, fabricar, distribuir y vender** el diseño y los
  productos basados en él. La **publicidad** está permitida.
- 🔁 **Recíproca fuerte (copyleft):** toda variación o derivado debe licenciarse
  también bajo CERN-OHL-S v2 — es decir, **toda variación sigue siendo hardware
  abierto**.
- 📝 Debes **conservar los avisos (Notices)** de copyright y autoría, e indicar tus
  modificaciones (fecha y descripción breve).
- 🔗 Debes **dar acceso al diseño fuente** (Source Location) del original y de tus
  variaciones; si fabricas un producto, debes entregar la fuente o notificar dónde
  obtenerla, y — si así se indica — mostrar la ubicación fuente en el producto o su
  empaque (sección 4).

**Aviso para incluir en el diseño / producto (Source Location = redirección al proyecto):**

```
Copyright 2026 Benjamín Garrido, José Tomás Romero

This source describes Open Hardware and is licensed under the CERN-OHL-S v2.
You may redistribute and modify this source and make products using it under
the terms of the CERN-OHL-S v2 (https://ohwr.org/cern_ohl_s_v2.txt).
This source is distributed WITHOUT ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING OF
MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS FOR A PARTICULAR PURPOSE.
Please see the CERN-OHL-S v2 for applicable conditions.
Source Location: https://github.com/widerllama2014/ecgsimulator
```
