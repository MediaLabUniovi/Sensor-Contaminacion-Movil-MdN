#include "LongPress.h"

// Constructor por defecto: inicializa el estado interno
LongPressState::LongPressState()
  : lastSampleTime(0)
  , pressStartTime(0)
  , stableCount(0)
  , lastRawState(HIGH)
  , debouncedState(HIGH)
  , longPressFired(false)
{}

// Implementación de la rutina de debounce + long‑press
LongPressResult checkLongPress(LongPressState &st, const LongPressConfig &cfg) {
  unsigned long now = millis();
  LongPressResult result = NO_EVENT;

  // 1) Muestreo periódico para debounce
  if (now - st.lastSampleTime >= cfg.sampleIntervalMs) {
    st.lastSampleTime = now;
    bool raw = digitalRead(cfg.buttonPin);

    if (raw == st.lastRawState) {
      if (st.stableCount < cfg.stableSamplesRequired) 
        st.stableCount++;
    } else {
      st.stableCount    = 1;
      st.lastRawState   = raw;
    }

    if (st.stableCount >= cfg.stableSamplesRequired && st.debouncedState != raw) {
      st.debouncedState = raw;
      if (raw == LOW) {
        st.pressStartTime = now;
        st.longPressFired = false;
      } else {
        if (!st.longPressFired) {
          result = PRESS_CANCELLED;
        }
      }
    }
  }

  // 2) Detección de long‑press
  if (st.pressStartTime != 0
      && !st.longPressFired
      && (now - st.pressStartTime >= cfg.longPressThresholdMs)) {
    st.longPressFired = true;
    result = LONG_PRESS_DETECTED;
  }

  // 3) Reset tras evento
  if (result != NO_EVENT) {
    st.stableCount     = 0;
    st.lastRawState    = HIGH;
    st.debouncedState  = HIGH;
    st.pressStartTime  = 0;
    st.longPressFired  = false;
  }

  return result;
}
