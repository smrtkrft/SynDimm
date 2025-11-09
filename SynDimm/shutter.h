/*
 * SynDimm - Shutter Control Library
 * Motorized shutter/blind control with position tracking
 * 
 * Powered by SEU - Emek - SmartKraft
 * Author: Smart Engineering Unit
 * https://github.com/smartkraft
 */

#ifndef SHUTTER_H
#define SHUTTER_H

#include <Arduino.h>
#include <Preferences.h>
#include "encoder.h"

class Shutter {
private:
  Encoder* encoder;
  Preferences prefs;
  
  // Shutter position (0-100)
  int position;  // 0 = fully closed, 100 = fully open
  
  // Motor control (placeholder for future implementation)
  bool motorRunning;
  unsigned long motorStartTime;
  
public:
  Shutter(Encoder* enc) : encoder(enc), position(50), motorRunning(false), motorStartTime(0) {}
  
  void begin() {
    prefs.begin("shutter", false);
    position = prefs.getInt("position", 50);  // Load saved position (default 50%)
    Serial.println("[Shutter] Initialized");
    Serial.print("[Shutter] Position: ");
    Serial.println(position);
  }
  
  // Process encoder events (up/down control)
  void processEncoderEvent(char event) {
    // TODO: Implement shutter control logic
    // L = Close, R = Open, B = Stop
    Serial.print("[Shutter] Event: ");
    Serial.println(event);
  }
  
  // Update shutter state (called in main loop)
  void update() {
    // TODO: Motor control, position tracking
  }
  
  // Getters
  int getPosition() { return position; }
  bool isMoving() { return motorRunning; }
  
  // Setters
  void setPosition(int pos) {
    if (pos >= 0 && pos <= 100) {
      position = pos;
      prefs.putInt("position", position);
    }
  }
  
  // Manual control
  void open() { setPosition(100); }
  void close() { setPosition(0); }
  void stop() { motorRunning = false; }
};

#endif
