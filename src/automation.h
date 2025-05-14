#ifndef AUTOMATION_H
#define AUTOMATION_H


enum State { IDLE, STARTED, DIRECTION_CHANGED, STOPPED };

State getCurrentAutomationState();

void initAutomation();
void updateAutomation(); // Wird in loop() aufgerufen
void startAutomationSequence();
void stopAutomationSequence();
bool isAutomationRunning();

#endif
