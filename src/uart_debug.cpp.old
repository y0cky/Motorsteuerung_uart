#include <Arduino.h>

// UART TX/RX Pins für ESP32-S2 Mini, passe bei Bedarf an
#define UART_RX 5   // Empfang von externem Endgerät (z.B. Motorcontroller)
#define UART_TX 7   // Senden zu externem Endgerät

void setup() {
  Serial.begin(115200);                        // USB-Seriell zum PC (Monitor)
  Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX); // Hardware UART (z.B. Motorsteuerung)
  Serial.println("UART-USB-Bridge gestartet");
}

void loop() {
  // Alles von Serial1 zu Serial (-> Monitor)
  while (Serial1.available()) {
    int ch = Serial1.read();
    Serial.write(ch);
  }

  // Alles von Serial (Monitor) zu Serial1 (-> Externes Gerät)
  while (Serial.available()) {
    int ch = Serial.read();
    Serial1.write(ch);
  }
}