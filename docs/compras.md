# Lista de compras / opciones de componentes

Opciones de compra evaluadas y seleccionadas para el prototipo (Parte II).
Todos los componentes son **THT** (coherente con las reglas de fabricación CNC).

> Los módulos se montan sobre la PCB mediante headers/pines; no se rediseñan sus
> circuitos internos. Precios y stock sujetos a cambios del proveedor.

## Módulos y conectores seleccionados

| Ref. | Componente | Modelo / detalle | Proveedor | Enlace |
|---|---|---|---|---|
| U1 | MCU | ESP32-S3-DevKitC-1-N16R8V (16 MB flash, 8 MB PSRAM) | — (ya disponible) | Librería KiCad: SnapEDA (ver abajo) |
| M1 | Cargador de batería | Módulo TP4056 USB-C 1A (con protección BMS) | vshop.cl | https://vshop.cl/modulos-arduino-esp32/1793-modulo-cargador-bateria-bms-de-litio-tp4056-usb-c-1a-esp32-arduino.html |
| M2 | Convertidor boost | Módulo DC-DC Step-Up MT3608 (→ 5 V) | Altronics | https://altronics.cl/modulo-conversor-dc-dc-booster-step-up-mt3608?search=MT3608 |
| J_BNC | Conector osciloscopio | BNC macho RG58 CN381 | electronicahobby.cl | https://electronicahobby.cl/producto/bnc-m-rg58-cn381 |
| M3 | Medidor de nivel de carga | Medidor de carga batería Li-Ion 1S–8S ajustable (verde) | vshop.cl | https://vshop.cl/modulos-arduino-esp32/1893-medidor-nivel-de-carga-bateria-li-ion-1-8s-ajustable-verde-esp32-arduino.html |

## Notas de diseño asociadas

- **M3 (medidor de carga) reemplaza al LED indicador de batería.** El estado de la
  batería ahora se visualiza con este módulo (display/LEDs propios), por lo que se
  elimina el LED D4 + R8 del esquemático. *(Pendiente: generar símbolo esquemático +
  footprint propios; hay un modelo STEP disponible para el 3D.)*
- **Se mantiene un LED de encendido (on/off).** Sirve para identificar visualmente si
  el equipo está prendido o apagado (D2 + R2 sobre el riel de 5 V conmutado).
- **LED de Bluetooth (D3 + R3)** controlado por GPIO del ESP32-S3 (conexión BLE OK).
- **Condensadores de desacople** C3 (100 µF) + C4 (100 nF) en la salida de 5 V del
  MT3608, lo más cerca posible del pin 5 V del DevKit, para absorber los picos de
  corriente del Bluetooth.

## Librería KiCad del MCU

- **ESP32-S3-DevKitC-1-N16R8V** — símbolo, footprint y 3D desde SnapEDA:
  https://www.snapeda.com/parts/ESP32-S3-DEVKITC-1-N16R8V/Espressif%20Systems/view-part/
  Se integra en `electronica/libraries/`.

## Alternativas evaluadas (no seleccionadas, referencia futura)

- **Cargador:** MCP73831 (LiPo Amigo Pro), MCP73871 / BQ24075 (power-path).
- **Boost:** TPS61023 / TPS61022 (síncronos, mejor eficiencia con batería baja).
- **Todo-en-uno:** IP5306 (cargador + boost 5 V + indicador en un chip), descartado por
  venir principalmente en SMD (choca con el requisito THT).
