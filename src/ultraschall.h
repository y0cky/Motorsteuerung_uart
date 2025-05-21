#ifndef ULTRASCHALL_H
#define ULTRASCHALL_H

void initUltraschall(int triggerPin, int echoPin);
long leseAbstand(); // Abstand in cm
int berechneRPMausAbstand(long abstand); // z.B. 55cm → 1150

#endif
