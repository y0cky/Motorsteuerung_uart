#include "automation.h"
#include <Arduino.h>

// Automatisierungszustände (jetzt in automation.h definiert)
State currentState = IDLE;

unsigned long lastStateChange = 0;
bool automationActive = false;

State getCurrentAutomationState() {
  return currentState;
}

void initAutomation() {
  currentState = IDLE;        // Startzustand
  automationActive = false;   // Automatisierung inaktiv
}

void startAutomationSequence() {
  automationActive = true;          // Automatisierung aktiv
  currentState = STARTED;           // Setze den Zustand auf STARTED
  lastStateChange = millis();       // Setze die Zeit, wann der Zustand geändert wurde
  Serial1.println("START");         // Sende START-Befehl
  Serial.println(String(millis()) + " ms: UART: START (Automation)");  // Zeigt die Zeit und Nachricht an
  
  // Setze die Drehzahl des Motors auf 1000 RPM (oder andere gewünschte Drehzahl)
  Serial1.println("SET_SPEED:1500");
  Serial.println(String(millis()) + " ms: UART: SET_SPEED:1500");
}

void stopAutomationSequence() {
  automationActive = false;      // Automatisierung stoppen
  currentState = IDLE;           // Setze den Zustand auf IDLE
  Serial1.println("STOP");       // Sende STOP-Befehl
  Serial.println(String(millis()) + " ms: UART: STOP (Automation)");  // Zeigt die Zeit und Nachricht an
}

bool isAutomationRunning() {
  return automationActive;
}

void updateAutomation() {
  if (!automationActive) return;  // Wenn Automatisierung nicht aktiv ist, abbrechen
  
  unsigned long now = millis();  // Aktuelle Zeit in ms

  switch (currentState) {
    case STARTED:
      // Wenn 2500 Milisekunden vergangen sind, wechsle den Zustand zu DIRECTION_CHANGED
      if (now - lastStateChange > 2500) {
        // STOP Befehl senden, aber nicht sofort Zustand wechseln
        Serial1.println("SET_SPEED:-750");  // Drehzahl ändern auf 750 RPM
        Serial.println(String(millis()) + " ms: UART: SET_SPEED:-750");
        // Nur den Zustand wechseln, wenn wir den STOP-Befehl gesendet haben
        currentState = DIRECTION_CHANGED;  // Zustand auf DIRECTION_CHANGED setzen
        lastStateChange = now;            // Zeitstempel für den Übergang setzen
      }
      break;

    case DIRECTION_CHANGED:
      // Wenn 5s vergangen sind, wechsle den Zustand zu STOPPED
      if (now - lastStateChange > 5000) {
        // Serial1.println("STOP"); 
        // Serial.println(String(millis()) + " ms: UART: STOP");
        
        // Hier nach dem SET_SPEED auf STOP wechseln
        currentState = STOPPED;            // Zustand auf STOPPED setzen
        lastStateChange = now;             // Zeitstempel für den Übergang setzen
      }
      break;

    case STOPPED:
      // Wenn 0,2 Sekunden vergangen sind, stoppe die Automatisierung
      if (now - lastStateChange > 200) {
        stopAutomationSequence();   // Automatisierung stoppen
        // Serial.println(String(millis()) + " ms: UART: STOP (Automation)");  // Zeigt die Zeit und Nachricht an
      }
      break;

    default:
      break;
  }
}
