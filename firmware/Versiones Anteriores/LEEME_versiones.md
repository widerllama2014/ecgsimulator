# Versiones anteriores del firmware

En orden cronológico. Cada `.ino` va en una carpeta con su mismo nombre, como exige el IDE de Arduino.

| Carpeta | Placa | Qué es |
|---|---|---|
| `tinkercad_ecg_uno` | Arduino UNO (Tinkercad) | Simulación del primer diseño: un PWM en el pin 9, R3 de 100 kΩ y red de 100 Ω en serie (15 oct) |
| `simulador_ecg_pwm` | ESP32-S3 | Primer programa completo: un canal, 4 ritmos, calibración, amplitud ×2 y botones |
| `ecg_beta_pin4` | ESP32-S3 | Versión mínima de prueba: solo el ECG por el GPIO 4, sin botones |
| `riyanto_replica` | Arduino Nano/UNO | Reconstrucción del circuito de Riyanto et al. (2025) a partir de su esquemático |
| `riyanto_esp32` | ESP32-S3 | El circuito de Riyanto adaptado a 3,3 V y a los componentes disponibles |
| `simulador_ecg_esp32cam` | ESP32-CAM | Primer programa en la ESP32-CAM: un canal con la red de la Parte I (29 oct) |
| `simulador_ecg_vector` | ESP32-CAM | Modo vector sin WiFi; con él se midieron las Figuras 27 a 29 del informe |
| `simulador_ecg_wifi_sin_emg` | ESP32-CAM | Control por WiFi antes del modo EMG; la versión que funcionó en la placa (agregarla desde el computador) |

Las versiones vigentes están en `firmware/simulador_ecg_2canales` y `firmware/simulador_ecg_wifi`.
