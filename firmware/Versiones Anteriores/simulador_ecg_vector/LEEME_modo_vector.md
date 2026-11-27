# Modo vector: electrodos movibles y vector cardíaco en 3D

Idea de José Tomás. Es un extra: **no reemplaza la entrega principal** y **no cambia el circuito**.

## Qué hace

- El corazón es una flecha H(t) en 3D que gira durante el latido (vectocardiograma).
- En el PC aparece un cuerpo con las tres pinzas (RA, LA, LL). **Se arrastran con el mouse.**
- Al soltar una pinza, el PC calcula qué "ve" ese electrodo en su nueva posición y se lo manda a la ESP32.
- La ESP32 cambia la señal de los bornes. El monitor de ECG muestra lo que mostraría con los electrodos puestos ahí.
- El PC grafica DI, DII y DIII, la flecha del corazón y su recorrido, de frente y en un modelo 3D que se gira.

## Archivos

| Archivo | Para qué |
|---|---|
| `firmware/simulador_ecg_vector.ino` | Código de la ESP32-CAM. Mismos pines y mismo circuito de 2 canales. |
| `pc/vector_ecg.py` | Programa del PC: cuerpo con pinzas arrastrables, curvas y modelo 3D. |
| `firmware/simulador_ecg_2canales.ino` | Código principal. No cambia; para volver a él basta con subirlo de nuevo. |

## Paso a paso

### 1. Probar sin la placa (5 minutos)
1. Instalar Python desde python.org. En Windows, marcar **"Add python.exe to PATH"** en el instalador.
2. Abrir una terminal (en Windows: tecla Windows, escribir `cmd`) y ejecutar:
   ```
   pip install pyserial matplotlib numpy
   ```
3. Ir a la carpeta donde está `vector_ecg.py` y ejecutar:
   ```
   python vector_ecg.py --demo
   ```
4. Arrastrar una pinza, probar las teclas `0` a `5` y girar el modelo 3D con el mouse.

### 2. Subir el código a la ESP32-CAM
1. Guardar `simulador_ecg_vector.ino` en una carpeta llamada `simulador_ecg_vector`.
2. Placa: **AI Thinker ESP32-CAM**. Sin tarjeta microSD. Subir.
3. Abrir el Monitor Serie a 115200 y pulsar RST. Debe aparecer:
   ```
   # PWM OK: 62500 Hz, 10 bits | MODO VECTOR v2 | eje normal = +41 grados
   # Escenario 0: Normal | 60 lpm
   ```
4. **Cerrar el Monitor Serie** (el puerto solo admite un programa a la vez).

### 3. Conectar el programa del PC
```
python vector_ecg.py --puerto COM5
```
Cambiar `COM5` por el puerto que muestra el Arduino IDE. Si la placa se reinicia, el programa vuelve a pedir los datos solo.

### 4. Controles

| Acción | Qué hace |
|---|---|
| Arrastrar una pinza | Mueve ese electrodo. Si se suelta cerca de su lugar normal, se pega a él. |
| `0` a `5` | Escenarios (tabla de abajo) |
| `←` `→` | Gira el corazón de a 15° |
| `↑` `↓` | Ritmo: 40, 60, 120 o 180 lpm |
| `c` | Calibración (1 mV, 1 Hz) |
| `r` | Devuelve las pinzas a su lugar |
| Mouse sobre el modelo 3D | Lo gira |

Sin el PC también funciona: en el Monitor Serie se escribe `0`–`5`, `eje 75` o `ritmo 2`. El botón de IO2, si está conectado, recorre los escenarios.

## Escenarios y valores esperados

| # | Escenario | R en DI | R en DII | R en DIII | Qué enseña |
|---|---|---|---|---|---|
| 0 | Normal (eje +41°) | +0,80 | +1,00 | +0,20 | Referencia |
| 1 | Eje a la izquierda (−45°) | +0,74 | −0,28 | −1,02 | DII y DIII negativas |
| 2 | Eje a la derecha (+115°) | −0,44 | +0,61 | +1,05 | DI negativa, DIII alta |
| 3 | Cables RA y LA invertidos | −0,80 | +0,20 | +1,00 | DI al revés; DII y DIII intercambiadas |
| 4 | Cables LA y LL invertidos | +1,00 | +0,80 | −0,20 | DI y DII intercambiadas; DIII invertida |
| 5 | Dextrocardia | −0,80 | +0,20 | +1,00 | En las extremidades se ve igual que el 3 |

Valores en mV.

**En el osciloscopio:** igual que en la guía de prueba (DC, X1, 500 mV/div, 200 ms/div), pero la línea base está en **1,55 V**. Altura en el TP = 1,55 V + (mV × 1,101). Con el multímetro: IO14 e IO13 ≈ 1,63 V, TP ≈ 1,55 V.

## Cómo funciona (para explicarlo)

1. **El corazón:** cada onda (P, Q, R, S, T) es una flecha con tres componentes: x hacia el brazo izquierdo, y hacia los pies, z hacia el pecho. Sumadas en el tiempo forman H(t).
2. **Cada electrodo** ve la proyección de la flecha: Φ = L · H. El vector L depende de dónde está el electrodo: L = (R0² / √3) · p / |p|³, con p la posición respecto del corazón.
3. **Cada borne** entrega la diferencia contra RA, porque RA está en GND:
   - borne LA = (L_LA − L_RA) · H → es la DI que ve el monitor
   - borne LL = (L_LL − L_RA) · H → es la DII
   - DIII sale sola: DII − DI
4. **Por eso bastan 2 canales:** el monitor solo mide diferencias. Un tercer canal solo sumaría lo mismo a todos los bornes, y eso el monitor no lo ve.
5. **En las posiciones normales** (los vértices del triángulo de Einthoven) el modelo da exactamente el ECG del simulador principal.
6. **Si una pinza queda muy cerca del corazón,** la señal crece más de lo que cabe en la salida (±1,35 mV por borne). La ESP32 la reduce en proporción y avisa en pantalla.

## Verificación hecha

| Prueba | Resultado |
|---|---|
| Compilación con el compilador real del ESP32 (paquete esp32 3.3.12, AI Thinker ESP32-CAM) | Sin errores ni advertencias |
| Firmware contra el modelo del PC, 17 secuencias de comandos | Diferencia menor que 0,001 µV |
| Circuito completo simulado (PWM → filtro RC → divisor → monitor), 13 casos | Error máximo 8 µV (0,8 % de 1 mV) |
| Firmware y programa del PC conectados por un puerto serie virtual, 11 pruebas | Todas correctas, incluido el reinicio de la placa |
| Ventana en vivo con mouse y teclado | Correcta, unos 18 cuadros por segundo |

**No está probado en la placa real ni con un monitor de ECG.** Eso es lo primero que hay que hacer.

## Límites

- **Modelo docente, no anatómico.** El cuerpo se trata como un medio uniforme y el corazón como un punto. Cerca del corazón exagera la amplitud.
- **La componente z es ilustrativa.** Sus valores se eligieron para que el pecho dé formas razonables (R chica y S profunda a la derecha del esternón, R alta a la izquierda). No vienen de un registro real.
- **Tres electrodos a la vez.** Para un ECG de 12 derivaciones harían falta más canales: una ESP32 DevKit tiene 16 salidas PWM.
- **Las pinzas van por delante del cuerpo.** No se pueden poner en la espalda.
- **Con el USB conectado al PC el simulador no está aislado.** Para conectarlo a un monitor real: power bank y sin el programa del PC, o bien un adaptador WiFi/Bluetooth más adelante.
- **Calibrar los dos canales** (`ATEN_LA` y `ATEN_LL`), igual que en el código principal. DIII depende de que estén iguales.

## Qué faltaría para ir más lejos

1. Electrodos precordiales (V1–V6) y central de Wilson: más canales y otra placa.
2. Enviar las posiciones por WiFi, para mover electrodos con el monitor conectado.
3. Un torso 3D con forma real en vez del cilindro.
4. Un vector cardíaco tomado de un vectocardiograma real.
