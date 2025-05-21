#include "ultraschall.h"
#include <Arduino.h>

static int trigPin;
static int echoPin;

void initUltraschall(int trigger, int echo) {
  trigPin = trigger;
  echoPin = echo;
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

long leseAbstand() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long dauer = pulseIn(echoPin, HIGH, 30000); // max 30ms Timeout
  long entfernung = dauer * 0.034 / 2;        // Umrechnung in cm
  return entfernung;
}

int berechneRPMausAbstand(long abstand) {
  if (abstand > 100) return 1500;            // >1m → volle Drehzahl
  else if (abstand < 10) return 800;         // <10cm → minimale Drehzahl
  else {
    // Lineare Interpolation zwischen 10cm = 800 RPM und 100cm = 1500 RPM
    return map(abstand, 10, 100, 800, 1500);
  }
}
