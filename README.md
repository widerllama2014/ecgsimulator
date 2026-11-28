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
| José Tomás Romero | jt.romeroreyes@udd.cl | Schematic, PCB, BOM, final prototype and Git repository |
| Benjamín Garrido | b.garridob@udd.cl | First prototype and first circuit design (on paper), firmware, software and measurements |

Shared by both: research and writing of the reports.

Professors: Alejandro Rebolledo and Wladimir Araya. Teaching assistant: Nicolas Walker.

---

## Status and presented version

- **Presented version:** the last commit of the `main` branch, running on the **PCB v2**
  board (milled in the lab and fixed by hand to match the v3 design).
- **Part I** (proposal and requirements) and **Part II** (electronic design and PCB):
  delivered, see `docs/informes/`.
- **Works:** tested on a breadboard with an oscilloscope: the four rhythms (40, 60, 120
  and 180 bpm), the 1 mV calibration pulse, the vector-mode scenarios and WiFi control.
  Results are in the final report.
- **PCB:** v2 milled and fixed by hand to match v3. v3 is the final design and was not
  milled again.
- **Pending:** oscilloscope test of the PCB, integration test with a real ECG reader
  (Project 4, groups 5 and 11), testing the EMG mode on hardware, a 10-measurement
  series per rhythm, and making the blue LED on IO12 work.

## Architecture

```
 POWER
   USB-C ──► TP4056 (charger) ──► SW1 ON/OFF ──► MT3608 (boost, set to 5.0 V) ──► 5 V ──► ESP32-CAM
                 ▲                                                                  └──► green LED (ON)
                 └── LiPo 1S 3.7 V, 1000 mAh (B+ / B−)

 SIGNAL (one RC filter and one 1:1101 divider per channel)
   ESP32-CAM ─┬─ IO14 (PWM) ──► RC filter ──► TP1 ──► 220 kΩ / 200 Ω ──► LA electrode
              └─ IO13 (PWM) ──► RC filter ──► TP2 ──► 220 kΩ / 200 Ω ──► LL electrode
                                     RA electrode ── 200 Ω ── GND       RL electrode ── GND

 INTERFACES
   SW2 (IO15) ──► change mode              GPIO33 ──► on-board red LED, blinks with each beat
   WiFi network "Simulador-ECG" ──► web page at 192.168.4.1 (phone or PC)
   USB serial, 115200 baud ──► Arduino Serial Monitor or software/vector_ecg.py
```

Each RC filter has two stages: 1 kΩ / 100 nF followed by 10 kΩ / 10 nF.

**The simulator works without any external application.** The whole signal is
generated on the ESP32-CAM, and the control web page is served by the board itself.
The PC tools in `software/` are optional.

**Main files:**

- **Schematic:** [KiCad](electronica/kicad/Proyecto%20de%20prueba.kicad_sch) ·
  [PDF](electronica/outputs/Esquematicopdf.pdf) · [PNG](electronica/outputs/imagenes/Esquematico.png)
- **PCB:** [KiCad](electronica/kicad/Proyecto%20de%20prueba.kicad_pcb) ·
  [PDF](electronica/outputs/PCBpdf.pdf) · [PNG](electronica/outputs/imagenes/PCB.png) ·
  [3D (STEP)](electronica/outputs/ECG.step)
- **Firmware:** [simulador_ecg_2canales.ino](firmware/simulador_ecg_2canales/simulador_ecg_2canales.ino) ·
  [simulador_ecg_wifi.ino](firmware/simulador_ecg_wifi/simulador_ecg_wifi.ino)
- **Software:** [vector_ecg.py](software/vector_ecg.py) ·
  [banco_virtual_ecg.html](software/banco_virtual_ecg.html)

### How the signal works

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
| ESP32-CAM (AI-Thinker) | Generates both PWM channels, creates its own WiFi network and serves the control web page | [esp32cam.pdf](docs/datasheets/esp32cam.pdf) |
| TP4056 module (USB-C) | Charges the battery | [TP4056.PDF](docs/datasheets/TP4056.PDF) |
| LiPo 1S, 3.7 V, 1000 mAh (LP803040) | Power supply, no mains connection | [LP803040](docs/datasheets/LP803040-1000mAh-datasheet.pdf) |
| MT3608 boost module | Raises the battery voltage to 5 V for the ESP32-CAM | [MT3608.pdf](docs/datasheets/MT3608.pdf) |
| KCD1-101F switch (SW1) | Power on/off | [KCD1-101F.pdf](docs/datasheets/KCD1-101F.pdf) |
| SW2 | Mode button (IO15) | |
| 4 × ECG snap connectors | Electrodes RA, LA, LL and RL, soldered on copper pads | |
| R 1 kΩ, 10 kΩ, 220 kΩ, 200 Ω; C 100 nF, 10 nF | RC filter and divider, one set per channel | |
| LEDs + 330 Ω | Green: power on (5 V). Blue: IO12 (not used by the current firmware) | |

The battery wires are soldered to the B+/B− pads of the TP4056 module.
**Set the MT3608 output to 5.0 V with its trimmer before plugging in the ESP32-CAM.**

### ESP32-CAM pins

| Pin | Use |
|---|---|
| IO14 | PWM, channel LA (DI) |
| IO13 | PWM, channel LL (DII) |
| IO15 | Mode button (to GND) |
| IO12 | Blue LED on the PCB. Not functional yet: the firmware reads IO12 as the start/stop button |
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

> **On the presented v2 board** the test points differ from v3. After the manual fix,
> the test point marked **TP1 carries channel LL**, and channel LA has no test point:
> measure it on the lead of **C5** that is not connected to GND. The GND test point is
> the same.

## PCB

- KiCad 10 project: `electronica/kicad/Proyecto de prueba.kicad_pro`. The design files
  are **v3**, the final version.
- 90 × 130 mm, single-sided, all tracks on **B.Cu** (no vias), designed for the lab CNC
  with 0.8 mm minimum track width and 0.8 mm clearance.
- The board milled in the lab is **v2**. It was brought up to v3 by hand: two track
  cuts to separate the channels, C7 replaced by a 200 Ω resistor (RA to GND) and a
  10 nF capacitor added to the LL channel. With this fix, v2 and v3 work the same way
  in practice; the only difference is that v2 has fewer test points.
- The electrode snaps were reused from disposable ECG electrodes, because we could not
  find them for sale. Each electrode was cut down to its snap, the top and bottom
  parts were separated, and only the top stud was mounted. Its material cannot be
  soldered, so it was fixed to the copper pad with white PVA glue (*cola fría*) on the
  edges only; the underside of the stud rests directly on the pad, and the electrical
  connection was checked.
- Four Ø3.2 mm mounting holes near the corners.
- The electrode snaps, both switches and the MT3608 module are mounted on the bottom
  side, which is the side facing the user.
- Custom symbols, footprints and 3D models are in `electronica/libraries/`
  (`proyecto_ECG.kicad_sym`, `proyecto_ECG.pretty/`, `3dmodels/`). All library and
  3D-model paths are relative to the project, so it opens on any computer.

| Fabrication file | Content |
|---|---|
| [`electronica/outputs/GERBER.zip`](electronica/outputs/GERBER.zip) | Gerbers (B.Cu, F.Cu, Edge.Cuts, F.Silkscreen) and drill files (PTH, NPTH) |
| [`electronica/outputs/MATERIALES_ECG.csv`](electronica/outputs/MATERIALES_ECG.csv) | Bill of materials (BOM) |
| [`electronica/outputs/ECG.step`](electronica/outputs/ECG.step) | 3D model of the assembled board |
| [`electronica/outputs/Esquematicopdf.pdf`](electronica/outputs/Esquematicopdf.pdf) | Schematic (PDF) |
| [`electronica/outputs/PCBpdf.pdf`](electronica/outputs/PCBpdf.pdf) | PCB views (PDF) |
| [`electronica/outputs/imagenes/`](electronica/outputs/imagenes) | Schematic and PCB (PNG), 3D renders (front and back) |

## Firmware (Arduino IDE)

- **Board:** AI Thinker ESP32-CAM, programmed through the **ESP32-CAM-MB** base.
- **Tools:** Arduino IDE 2 with the board package **esp32 by Espressif Systems**
  (tested with version 3.3.12).
- **Dependencies:** none beyond the board package. `WiFi.h` and `DNSServer.h` come
  with it.

**To compile and upload:**

1. In Arduino IDE, open *File → Preferences* and add this URL to *Additional boards
   manager URLs*: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Open *Tools → Board → Boards Manager* and install **esp32 by Espressif Systems**.
3. Open the sketch, for example
   `firmware/simulador_ecg_2canales/simulador_ecg_2canales.ino`. For the WiFi version,
   `pagina.h` must stay in the same folder as `simulador_ecg_wifi.ino`.
4. Remove the microSD card, plug the ESP32-CAM into the ESP32-CAM-MB base and connect
   it to the computer by USB.
5. In *Tools → Board*, choose **AI Thinker ESP32-CAM**, and in *Tools → Port*, the COM
   port of the base.
6. Click *Upload*. The base resets the board by itself, so no buttons are needed.
7. Open the Serial Monitor at **115200 baud** and press the RST button on the board.

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
| [`software/vector_ecg.py`](software/vector_ecg.py) | Desktop viewer for the vector mode: drag the electrodes on a body and see DI, DII, DIII and the heart vector in 3D |
| [`software/banco_virtual_ecg.html`](software/banco_virtual_ecg.html) | Virtual test bench: simulated board, circuit, oscilloscope and multimeter, to practice measurements before the lab |

**vector_ecg.py**

- **Dependencies:** Python 3 and `pip install pyserial matplotlib numpy`.
- **Without the board:** `python vector_ecg.py --demo`
- **With the board:** load `simulador_ecg_wifi` (it accepts the same serial commands as
  the earlier vector-mode sketch), connect the board by USB, **close the Arduino Serial
  Monitor**, and run `python vector_ecg.py --puerto COM5`. Use the board's port (on
  Mac/Linux, something like `/dev/ttyUSB0`). The program talks to the board over USB
  serial at 115200 baud.

**banco_virtual_ecg.html** opens in any web browser and does not connect to the board.

## Initial check

After uploading the firmware and turning the simulator on, you should see:

1. **Power:** with SW1 on, the green LED lights up.
2. **Serial Monitor (115200 baud):** with the 2-channel firmware, it prints
   `PWM OK: 62500 Hz, 10 bits, 2 canales (IO14 = LA, IO13 = LL)` and then
   `Modo: 60 lpm | R: DI 0.80, DII 1.00, DIII 0.20 mV`. The simulator starts at 60 bpm
   and the red LED on the ESP32-CAM blinks with each beat.
3. **Multimeter (DC):** about **0.80 V** at each channel's test point (1.55 V with the
   WiFi firmware). On the v2 board, see the note under *Test points*.
4. **Oscilloscope:** DC coupling, ×1 probe, 500 mV/div and 200 ms/div, with the ground
   clip on the GND test point. Channel LA swings between about 0.76 and 1.68 V and
   channel LL between about 0.50 and 1.90 V, one beat per second.
5. **Mode button:** each press of SW2 moves to the next mode (40, 60, 120, 180 bpm and
   calibration), and the Serial Monitor confirms the change.
6. **WiFi firmware:** the network **Simulador-ECG** (password `ecg12345`) appears.
   Open **http://192.168.4.1**: the page shows DI, DII and DIII and says
   **PLACA CONECTADA**.

To feed an ECG monitor, connect each monitor cable to the snap with the same name (RA,
LA, LL and RL) and run the simulator from its battery.

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
│   └── outputs/                BOM (CSV), STEP, Gerber + drill (.zip), schematic and PCB (PDF)
│       └── imagenes/           Schematic and PCB (PNG), 3D renders
├── firmware/                   ESP32-CAM sketches (current and previous versions)
└── software/                   PC tools: vector viewer and virtual bench
```

## References

- Riyanto, I. et al. (2025). Design and construction of an ECG simulator with bipolar
  leads based on the Arduino Nano microcontroller. https://doi.org/10.51747/energy.v15i2.15214
- Lynch, J. P. (2013). ECG simulator. https://www.instructables.com/ECG-Simulator/
- Microchip Technology (2020). Using PWM to generate an analog output (TB3250).
- Malmivuo, J. & Plonsey, R. (1995). *Bioelectromagnetism*. https://www.bem.fi/book/

### Sources of symbols, footprints and 3D models

| Part | Symbol and footprint | 3D model |
|---|---|---|
| ESP32-CAM | SnapMagic Search (SnapEDA): [ESP32-CAM by AI-Thinker](https://www.snapeda.com/parts/ESP32-CAM/AI-Thinker/view-part/) | Jan Ježek, GrabCAD: [ESP32-CAM](https://grabcad.com/library/esp32-cam-1) |
| MT3608 boost module | SnapMagic Search (SnapEDA): [MT3608 DC-DC Step Up Converter 2A Booster Module](https://www.snapeda.com/parts/MT3608%20DC-DC%20Step%20Up%20Converter%202A%20Booster%20Module/Generic/view-part/) | MRMS - WORKSHOP, GrabCAD: [MT3608 DC to DC Boost Converter 2 A](https://grabcad.com/library/mt3608-dc-to-dc-boost-converter-2-a-1) |
| TP4056 module (USB-C) | Made by the team (`TP4056_modulo`) | GrabCAD: *TP4056 Charging Module Type C* (STEP and Fusion 360) |
| KCD1 switch (SW1, SW2) | Made by the team (`SW_Push_2P_ECG`), measured from the physical part | Source unknown: the download record was lost |
| Electrode snap (J1–J4) | Made by the team (`conectorelectrodo`), based on KiCad's `TestPoint_Pad_D4.0mm` with the pad enlarged to 9 mm | 3D ContentCentral: [ECG Snap Button 4mm](https://www.3dcontentcentral.com/Download-Model.aspx?catalogid=171&id=1354647) |
| Resistors, capacitors, LEDs and test points | [KiCad official libraries](https://gitlab.com/kicad/libraries/kicad-footprints) | KiCad official 3D models |

## Acknowledgements

We thank:

- The **Engineering Materials Laboratory** (Laboratorio de Materiales de Ingeniería),
  for lending us instruments and a place to work.
- **Pablo Reyes**, for the ECG electrodes and for lending us an oscilloscope.
- The **Centro de Investigación en Tecnologías para la Sociedad (C+)**, for lending us
  materials and supplies, and for helping us with our questions.
- The **UDD Science Laboratory** (Laboratorio de Ciencias), for lending us the laboratory
  to work in and for providing supplies when we needed them.

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

## AI use statement

We used **Claude** (Anthropic) for code and research, and **Gemini** (Google) for
research as well. Claude also made the first versions of the schematic and the PCB,
which saved us a lot of time, and generated some of the symbols. In the end, all of
this was modified by us to fit what the project needed.
