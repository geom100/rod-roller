#ifndef FurryOwl_Debouncer_h
#define FurryOwl_Debouncer_h

#include <Arduino.h>

template<uint16_t TIMEOUT, uint8_t COUNT>
class Debouncer {
public:
  Debouncer()
    : _counter(0), _timer(millis()) {}
    
  int8_t event(int8_t dir) {
    if (_counter == 0 && millis() - _timer <= TIMEOUT) {
      _timer = millis();
      return 0;
    } else {
      if (millis() - _timer > TIMEOUT) {
        _counter = 1;
        _timer = millis();
        return 0;
      } else if (++_counter >= COUNT) {
        _counter = 0;
        _timer = millis();
        return dir;
      } else {
        return 0;
      }
    }
  }
private:
  uint8_t _counter;
  uint32_t _timer;
};

#endif