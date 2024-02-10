#ifndef FurryOwl_RodRoller_h
#define FurryOwl_RodRoller_h

#include <Arduino.h>

#define EB_NO_FOR       // отключить поддержку pressFor/holdFor/stepFor и счётчик степов (экономит 2 байта оперативки)
#define EB_NO_CALLBACK  // отключить обработчик событий attach (экономит 2 байта оперативки)
#define EB_NO_COUNTER   // отключить счётчик энкодера (экономит 4 байта оперативки)
#define EB_NO_BUFFER    // отключить буферизацию энкодера (экономит 1 байт оперативки)

// #define EB_DEB_TIME 50      // таймаут гашения дребезга кнопки (кнопка)
// #define EB_CLICK_TIME 500   // таймаут ожидания кликов (кнопка)
// #define EB_HOLD_TIME 600    // таймаут удержания (кнопка)
// #define EB_STEP_TIME 200    // таймаут импульсного удержания (кнопка)
#define EB_FAST_TIME 100  // таймаут быстрого поворота (энкодер)

// #define OLED_NO_PRINT

#include <EncButton.h>
#include <GyverOLED.h>
#include <Debouncer.h>
#include <ByteState.h>

namespace Pins {
    constexpr uint8_t PIN_STEPPER_STEP = 3;
    constexpr uint8_t PIN_STEPPER_DIR = 2;
    constexpr uint8_t PIN_STEPPER_ENA = 7;
    constexpr uint8_t PIN_STEPPER_M0 = 6;
    constexpr uint8_t PIN_STEPPER_M1 = 5;
    constexpr uint8_t PIN_STEPPER_M2 = 4;

    constexpr uint8_t PIN_ENC_1 = A0;
    constexpr uint8_t PIN_ENC_2 = A1;
    constexpr uint8_t PIN_ENC_KEY = A2;
}

class Stepper {
public:
    void init();

    void rpm(int8_t updown, bool fast);
    void dir(bool dir);
    void ena(bool ena);

    uint8_t currentRPM() {
        return _stepperRPM;
    }

    static constexpr uint8_t MIN_RPM = 1;
    static constexpr uint8_t MAX_RPM = 50;
    static constexpr uint8_t DEFAULT_RPM = 30;
    static constexpr int STEPS_PER_TURN = 3200;

private:
  uint32_t delayFromRPM();
  int fastStep();

  uint8_t _stepperRPM = DEFAULT_RPM;
};

typedef GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> RROLED;
typedef EncButtonT<Pins::PIN_ENC_1, Pins::PIN_ENC_2, Pins::PIN_ENC_KEY> RREncoder;

class RodRoller {
public:
  /**
   * Initialize roller in setup().
  */
  void init();

  /**
   * Call repeatedly in loop().
  */
  void tick();

protected:
  void updateOLEDState();

private:
  uint32_t _oledTimer = 0;

  ByteState _state = 0;

  static constexpr uint8_t _STEPPER_ENABLE_BIT = 0;
  static constexpr uint8_t _STEPPER_DIR_BIT = 1;
  static constexpr uint8_t _STEPPER_STATE_BIT = 2;
  static constexpr uint8_t _OLED_UPDATE_BIT = 3;
  static constexpr uint8_t _EEPROM_UPDATE_BIT = 4;

  RREncoder _encoder;
  RROLED _oled;
  Stepper _stepper;

  Debouncer<300, 5> _debouncer;
};

#endif