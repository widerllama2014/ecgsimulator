# 3-Lead ECG Simulator (ESP32-CAM)

Low-cost, open-hardware simulator that outputs the limb leads **DI, DII and DIII** at
millivolt level, as a safe test source for ECG monitors and readers. An ESP32-CAM
generates two PWM signals; each one goes through a two-stage RC filter and a 1:1101
voltage divider to reach physiological amplitude.

> **Not a medical device. Never connect it to a person.** It is meant to feed ECG
> monitors, acquisition boards and oscilloscopes for testing and teaching.

Project 5, *Biomedical Signal Simulator*, group 13. Electrónica Biomédica
(IBB224S-1), Universidad del Desarrollo (UDD), Chile, 2026.

| Team member | E-mail | Main role |
|---|---|---|
| José Tomás Romero | jt.romeroreyes@udd.cl | Schematic, PCB, BOM, documentation and repository |
| Benjamín Garrido | b.garridob@udd.cl | Firmware, user interface and measurements |

Instructors: Alejandro Rebolledo and Wladimir.

---

## How it works

```
                 ┌── IO14 ─ RC filter (1 kΩ/100 nF + 10 kΩ/10 nF) ─ TP1 ─ 220 kΩ ─┬─ LA
 ESP32-CAM ──────┤                                                                 └─ 200 Ω ─ GND
 (2 × PWM,       └── IO13 ─ RC filter (1 kΩ/100 nF + 10 kΩ/10 nF) ─ TP2 ─ 220 kΩ ─┬─ LL
  62.5 kHz,                                                                        └─ 200 Ω ─ GND
  10 bit)                                                         RA ─ 200 Ω ─ GND     RL ─ GND
```

- Each channel turns the PWM into a smooth voltage (both filter stages cut at ≈1.6 kHz)
  and divides it by **1101**, so **1 mV at the electrode = 1.101 V at the test point**.
- An ECG monitor only measures differences between electrodes, so two channels are
  enough for the three limb leads:
  - **DI = LA − RA** (channel 1)
  - **DII = LL − RA** (channel 2)
  - **DIII = LL − LA**, computed by the monitor itself, so Einthoven's law
    (DII = DI + DIII) always holds.
- RA is the zero of the lead differences (200 Ω to GND, the same output impedance as
  LA and LL). RL is the monitor's reference and goes straight to GND.
- The heartbeat is a sum of Gaussian waves (P, Q, R, S and T) stored as a table and
  played back one sample per millisecond from a 1 ms hardware timer.

## Hardware

| Part | Function | Datasheet |
|---|---|---|
| ESP32-CAM (AI-Thinker) | Generates both PWM channels; WiFi access point | [esp32cam.pdf](docs/datasheets/esp32cam.pdf) |
| TP4056 module (USB-C) | Charges the battery | [TP4056.PDF](docs/datasheets/TP4056.PDF) |
| LiPo 1S, 3.7 V, 1000 mAh (LP803040) | Power supply, no mains connection | [LP803040](docs/datasheets/LP803040-1000mAh-datasheet.pdf) |
| MT3608 boost module | Raises the battery voltage to 5 V for the ESP32-CAM | [MT3608.pdf](docs/datasheets/MT3608.pdf) |
| KCD1-101F switch (SW1) | Power on/off | [KCD1-101F.pdf](docs/datasheets/KCD1-101F.pdf) |
| SW2 | Mode button (IO15) | |
| 4 × ECG snap connectors | Electrodes RA, LA, LL and RL, soldered on copper pads | |
| R 1 kΩ, 10 kΩ, 220 kΩ, 200 Ω; C 100 nF, 10 nF | RC filter and divider, one set per channel | |
| LEDs + 330 Ω | Green: power on (5 V). Blue: status (IO12) | |

The battery wires are soldered to the B+/B− pads of the TP4056 module.
**Set the MT3608 output to 5.0 V with its trimmer before plugging in the ESP32-CAM.**

### ESP32-CAM pins

| Pin | Use |
|---|---|
| IO14 | PWM, channel LA (DI) |
| IO13 | PWM, channel LL (DII) |
| IO15 | Mode button (to GND) |
| IO12 | Blue LED on the PCB |
| GPIO33 | On-board red LED, blinks with each R wave |
| IO4 | Camera flash, kept off |

The camera and the microSD slot use most of the remaining pins: run the simulator
**without a microSD card** and do not press buttons while the board boots, because
IO2, IO12 and IO15 are strapping pins.

### Test points

| Test point | Signal |
|---|---|
| TP1 | Channel LA, in volts |
| TP2 | Channel LL, in volts |
| TP3 | GND, for the oscilloscope ground clip |

The electrodes are at millivolt level, so measure at the test points and convert:
`V_TP = baseline + 1.101 × mV`. The baseline is 0.80 V with the 2-channel firmware
and 1.55 V with the vector/WiFi firmware. For DIII, measure between TP1 and TP2 with a
battery-powered oscilloscope.

## PCB (ECG Simulator v3)

- KiCad 10 project: `electronica/kicad/Proyecto de prueba.kicad_pro`.
- 90 × 130 mm, single-sided, all tracks on **B.Cu** (no vias), milled on the lab CNC
  with 0.8 mm minimum track width and 0.8 mm clearance.
- Four Ø3.2 mm mounting holes near the corners.
- The electrode snaps, both switches and the MT3608 module are mounted on the bottom
  side, which is the side facing the user.
- Custom symbols, footprints and 3D models are in `electronica/libraries/`
  (`proyecto_ECG.kicad_sym`, `proyecto_ECG.pretty/`, `3dmodels/`). All library and
  3D-model paths are relative to the project, so it opens on any computer.
- **Design note:** the GND net is split in two parts that are joined through ground
  pins 2 and 9 of the ESP32-CAM. The module must be installed for all electrodes to
  share the same ground.

| Fabrication file | Content |
|---|---|
| [`electronica/outputs/GERBER.zip`](electronica/outputs/GERBER.zip) | Gerbers (B.Cu, F.Cu, Edge.Cuts, F.Silkscreen) and drill files (PTH, NPTH) |
| [`electronica/outputs/MATERIALES_ECG.csv`](electronica/outputs/MATERIALES_ECG.csv) | Bill of materials (BOM) |
| [`electronica/outputs/ECG.step`](electronica/outputs/ECG.step) | 3D model of the assembled board |
| [`electronica/outputs/imagenes/`](electronica/outputs/imagenes) | Schematic and PCB in PDF, 3D renders (top and bottom) |

## Firmware (Arduino IDE)

Board: **AI Thinker ESP32-CAM** (Espressif esp32 core 3.x). Upload through the
**ESP32-CAM-MB** base. Serial monitor at 115200 baud.

| Sketch | What it does |
|---|---|
| [`firmware/simulador_ecg_2canales`](firmware/simulador_ecg_2canales) | Main program: 40, 60, 120 and 180 bpm, 1 mV / 1 Hz calibration pulse, ×1/×2 amplitude, start/stop, beat LED |
| [`firmware/simulador_ecg_wifi`](firmware/simulador_ecg_wifi) | Cardiac-vector mode plus a web page served over WiFi (see its [LEEME_wifi.md](firmware/simulador_ecg_wifi/LEEME_wifi.md)) |
| [`firmware/Versiones Anteriores`](firmware/Versiones%20Anteriores) | Earlier versions, in chronological order ([LEEME_versiones.md](firmware/Versiones%20Anteriores/LEEME_versiones.md)) |

**Vector mode.** The heart is modelled as a 3D dipole; each electrode sees its
projection, so the same circuit shows six scenarios: normal axis, left axis deviation,
right axis deviation, RA–LA cables swapped, LA–LL cables swapped and dextrocardia. The
axis can be rotated freely and the electrodes can be moved.

**WiFi control.** The board creates its own network, **Simulador-ECG** (password
`ecg12345`). Open **http://192.168.4.1** on a phone or computer to see DI, DII and DIII,
drag the electrodes, change rhythm and scenario, and start or stop the output. No
internet is needed, so the simulator can run on battery, isolated from mains, while it
feeds a monitor.

**EMG mode (experimental).** A surface-EMG burst (1 s on, 1 s off, 20–450 Hz,
≈0.2 mV RMS) on channel LA. It is programmed but not yet tested on hardware.

## Software

| File | What it does |
|---|---|
| [`software/vector_ecg.py`](software/vector_ecg.py) | Desktop viewer for the vector mode: drag the electrodes on a body and see DI, DII, DIII and the heart vector in 3D. `pip install pyserial matplotlib numpy`, then `python vector_ecg.py --demo` (no board) or `python vector_ecg.py --puerto COM5` |
| [`software/banco_virtual_ecg.html`](software/banco_virtual_ecg.html) | Virtual test bench: simulated board, circuit, oscilloscope and multimeter, to practice measurements before the lab. Open it in any browser |

## Repository structure

```
.
├── README.md
├── LICENSE                     CERN-OHL-S v2
├── docs/
│   ├── bitacora.md             Development log
│   ├── datasheets/             Component datasheets
│   └── informes/               Reports: Part I, Part II and final report (PDF)
├── electronica/
│   ├── kicad/                  KiCad project (.kicad_pro, .kicad_sch, .kicad_pcb)
│   ├── libraries/              Custom symbols, footprints and 3D models
│   └── outputs/                BOM (CSV), STEP, Gerber + drill (.zip)
│       └── imagenes/           Schematic PDF, PCB PDF and 3D renders
├── firmware/                   ESP32-CAM sketches (current and previous versions)
└── software/                   PC tools: vector viewer and virtual bench
```

## Project status

- **Part I** (proposal and requirements): delivered, see `docs/informes/`.
- **Part II** (electronic design and PCB): delivered, see `docs/informes/`.
- **Final prototype:** tested on a breadboard with an oscilloscope (four rhythms,
  calibration pulse, vector-mode scenarios and WiFi control). Results are in the final
  report.
- **PCB v3:** milled on the lab CNC.
- **Pending:** integration test with a real ECG reader (Project 4, groups 5 and 11),
  testing the EMG mode on hardware and a 10-measurement series per rhythm.

## References

- Riyanto, I. et al. (2025). Design and construction of an ECG simulator with bipolar
  leads based on the Arduino Nano microcontroller. https://doi.org/10.51747/energy.v15i2.15214
- Lynch, J. P. (2013). ECG simulator. https://www.instructables.com/ECG-Simulator/
- Microchip Technology (2020). Using PWM to generate an analog output (TB3250).
- Malmivuo, J. & Plonsey, R. (1995). *Bioelectromagnetism*. https://www.bem.fi/book/

## License

This project is open hardware under the **CERN Open Hardware Licence Version 2 –
Strongly Reciprocal (CERN-OHL-S v2)**. See [LICENSE](LICENSE).

You may use, study, modify, manufacture, distribute and sell the design and products
based on it, as long as every derivative is released under the same licence, the
copyright notices are kept, modifications are stated, and the design source is made
available.

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
