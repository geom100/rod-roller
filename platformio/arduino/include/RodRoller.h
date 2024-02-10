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

typedef GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> OLED;

#define PIN_STEPPER_STEP 3
#define PIN_STEPPER_DIR 2
#define PIN_STEPPER_ENA 7
#define PIN_STEPPER_M0 6
#define PIN_STEPPER_M1 5
#define PIN_STEPPER_M2 4

#define PIN_ENC_1 A0
#define PIN_ENC_2 A1
#define PIN_ENC_KEY A2

#define MIN_RPM 1
#define MAX_RPM 50
#define DEFAULT_RPM 30
#define STEPS_PER_TURN (3200)

enum class StepperDir {
  Toggle = 0,
  Forward = 1,
  Backward = -1,
  None = 127
};

class RodRoller {
public:
  RodRoller();

  void init();

  void tick();

protected:
  uint32_t delayFromRPM();
  int fastStep();

  template <StepperDir dir>
  void changeStepperDir();

  void toggleStepper();


  void updateOLEDState();

private:
  uint8_t _stepperRPM = DEFAULT_RPM;

  uint32_t _oledTimer = 0;

  ByteState _state = 0;

  static constexpr uint8_t _STEPPER_ENABLE_BIT = 0;
  static constexpr uint8_t _STEPPER_DIR_BIT = 1;
  static constexpr uint8_t _STEPPER_STATE_BIT = 2;
  static constexpr uint8_t _OLED_UPDATE_BIT = 3;
  static constexpr uint8_t _EEPROM_UPDATE_BIT = 4;

  EncButton _encoder;
  OLED _oled;

  Debouncer<300, 5> _debouncer;
};

#endif