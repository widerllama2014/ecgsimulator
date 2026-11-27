# Simulador de ECG con página web por WiFi

Es el modo vector, con el mismo circuito, más una página para controlarlo desde el teléfono o el computador.
La placa crea su propia red WiFi. No necesita internet ni cable al PC.

## Archivos

| Archivo | Para qué |
|---|---|
| `simulador_ecg_wifi.ino` | Programa de la ESP32-CAM |
| `pagina.h` | La página web. Va dentro del chip. Debe estar en la misma carpeta que el `.ino` |

## Pasos

1. Dejar los dos archivos en una carpeta llamada `simulador_ecg_wifi`.
2. Abrir `simulador_ecg_wifi.ino` en el Arduino IDE. `pagina.h` aparece como segunda pestaña.
3. Placa: **AI Thinker ESP32-CAM**. Sin tarjeta microSD. Subir.
4. Abrir el Monitor Serie a 115200 y pulsar RST. Debe aparecer:
   ```
   # PWM OK: 62500 Hz, 10 bits | MODO VECTOR v2 + WiFi | eje normal = +41 grados
   # WiFi listo. Red "Simulador-ECG", clave "ecg12345". Abrir http://192.168.4.1
   ```
5. En el teléfono o el computador, conectarse a la red **Simulador-ECG** (clave **ecg12345**).
6. Abrir el navegador en **http://192.168.4.1**. En muchos teléfonos la página se abre sola.
7. Arriba a la derecha debe decir **PLACA CONECTADA**.

Si el teléfono avisa que la red no tiene internet, elegir "mantener conexión". En algunos hay que apagar los datos móviles.

## Qué se puede hacer en la página

| Control | Qué hace |
|---|---|
| Botones 0 a 5 | Escenarios: normal, eje izquierdo, eje derecho, cables invertidos, dextrocardia |
| Deslizador | Gira el corazón a cualquier ángulo |
| 40 · 60 · 120 · 180 · CAL | Ritmo, o calibración de 1 mV |
| Arrastrar una pinza | Mueve ese electrodo en el cuerpo |
| FRENTE · 3D | Cambia la vista del paciente |
| DETENER / INICIAR | Detiene o reanuda la salida |
| TEMA | Cambia entre clínica (rojo y blanco) y rayo (negro y amarillo) |

Pueden conectarse varios equipos a la vez. Lo que cambia uno aparece en los demás en un par de segundos.

## Cambiar los datos de la ficha

Abrir la pestaña `pagina.h`. Cerca de la línea 14 está el bloque `FICHA` con universidad, carrera, curso, proyecto e integrantes. Se cambia el texto entre comillas y se vuelve a subir.

## Poner el logo

- **Rápido:** tocar el recuadro LOGO en la página y elegir la imagen. Queda guardada en ese navegador.
- **Fijo en la placa:** pegar la imagen como texto `data:image/png;base64,...` en `logo:` dentro de `FICHA`.

## Cambiar el nombre o la clave de la red

Al principio del `.ino`: `WIFI_NOMBRE` y `WIFI_CLAVE` (8 caracteres o más). Con `USAR_WIFI = false` la placa funciona sin red, igual que el modo vector.

## Verificación hecha

| Prueba | Resultado |
|---|---|
| Compilación con el compilador real del ESP32 (paquete esp32 3.3.12) | Sin errores ni advertencias |
| Tablas de salida contra el modo vector ya probado en la placa, 11 casos | Idénticas |
| Programa ejecutado en el PC con su servidor web y la página en un navegador, 34 pruebas | Todas correctas |

Lo que cubren esas 34 pruebas: botones, deslizador, teclado, arrastre de pinzas, dos equipos a la vez, corte y reconexión, conexiones que no piden nada, pedidos mal escritos.

**No está probado en la placa real.** Falta comprobar ahí la red WiFi y que la señal siga limpia en el osciloscopio.

## Si algo falla

| Síntoma | Qué probar |
|---|---|
| La placa se reinicia al partir | El WiFi consume más. Alimentar por el pin de 5 V o usar otro cable o fuente |
| No aparece la red | Mirar el Monitor Serie. Si no dice "WiFi listo", pulsar RST |
| La página dice SIN CONEXION | El equipo se salió de la red. Volver a conectarse a Simulador-ECG |
| La señal se ve con más ruido que antes | Comparar con `USAR_WIFI = false`. Si mejora, el ruido viene de la radio |

Los comandos por el Monitor Serie y los botones siguen funcionando. El programa `vector_ecg.py` también.
