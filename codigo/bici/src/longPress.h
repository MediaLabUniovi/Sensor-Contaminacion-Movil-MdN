#ifndef LONGPRESS_H
#define LONGPRESS_H

#include <Arduino.h>

// Resultado de la comprobación
enum LongPressResult : int8_t {
  NO_EVENT = 0,
  LONG_PRESS_DETECTED,
  PRESS_CANCELLED
};

// Parámetros de configuración
struct LongPressConfig {
  uint8_t        buttonPin;
  unsigned long  sampleIntervalMs;
  uint8_t        stableSamplesRequired;
  unsigned long  longPressThresholdMs;
};

// Estado mutable entre llamadas
struct LongPressState {
  unsigned long lastSampleTime;
  unsigned long pressStartTime;
  uint8_t       stableCount;
  bool          lastRawState;
  bool          debouncedState;
  bool          longPressFired;

  LongPressState();
};

// Prototipo de la función
LongPressResult checkLongPress(LongPressState &st, const LongPressConfig &cfg);

#endif // LONGPRESS_H
