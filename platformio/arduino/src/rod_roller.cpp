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
#include <GyverTimers.h>

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

// #define MYDEBUG 1

#ifdef MYDEBUG
uint32_t debugTimer = 0;
char debug_buffer[128];
#endif

const static uint8_t PROGMEM _s_icons[][7] =
{
    {0x40, 0x20, 0x11, 0x09, 0x05, 0x03, 0x1F}, // Стрелка вверх 
    {0x7C, 0x60, 0x50, 0x48, 0x44, 0x02, 0x01} // Стрелка вниз 
};


enum class StepperDir {
  Toggle = 0,
  Forward = 1,
  Backward = -1,
  None = 127
};

enum class StateOp {
  On = 0,
  Off = 1,
  Toggle = 2
};

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


class RodRoller {
public:
  RodRoller()
    : _encoder(PIN_ENC_1, PIN_ENC_2, PIN_ENC_KEY, INPUT, INPUT_PULLUP, LOW) {}

  void init() {
    pinMode(PIN_STEPPER_STEP, OUTPUT);
    pinMode(PIN_STEPPER_DIR, OUTPUT);
    pinMode(PIN_STEPPER_ENA, OUTPUT);
    pinMode(PIN_STEPPER_M0, OUTPUT);
    pinMode(PIN_STEPPER_M1, OUTPUT);
    pinMode(PIN_STEPPER_M2, OUTPUT);

    digitalWrite(PIN_STEPPER_ENA, !(isState(_STEPPER_ENABLE_BIT)));
    digitalWrite(PIN_STEPPER_DIR, isState(_STEPPER_DIR_BIT));

    digitalWrite(PIN_STEPPER_M0, 0);
    digitalWrite(PIN_STEPPER_M1, 0);
    digitalWrite(PIN_STEPPER_M2, 1);

    Timer2.outputEnable(CHANNEL_B, TOGGLE_PIN);
    Timer2.setPeriod(delayFromRPM());
    Timer2.stop();

    _encoder.setEncType(EB_STEP4_LOW);
    _encoder.setEncReverse(true);

    _oled.init();   // инициализация
    _oled.clear();  // очистка

    _oled.roundRect(0, 0, 88, 31, OLED_STROKE);
    updateOLEDState();

#ifdef MYDEBUG
    Serial.println("Starting...");
#endif
  }

  void tick() {
    if (_encoder.tick()) {
      if (_encoder.turn() && _encoder.pressing()) {
        int newRpm = _stepper_rpm + (_encoder.fast() ? fastStep() : 1) * _encoder.dir();
        _stepper_rpm = constrain(newRpm, MIN_RPM, MAX_RPM);
        Timer2.setPeriod(delayFromRPM());
        setState<StateOp::On>(_OLED_UPDATE_BIT);
      } else if (_encoder.turn()) {
        int dir = _debouncer.event(_encoder.dir());
        if (isState(_STEPPER_ENABLE_BIT)) {
#ifdef MYDEBUG
          sprintf(debug_buffer, "Stepper is on, encoder dir %d", dir);
          Serial.println(debug_buffer);
#endif
          if (dir > 0) {
            changeStepperDir<StepperDir::Toggle>();
            setState<StateOp::On>(_OLED_UPDATE_BIT);
          } else if (dir < 0) {
            toggleStepper();
            setState<StateOp::On>(_OLED_UPDATE_BIT);
          }
#ifdef MYDEBUG
            sprintf(debug_buffer, "On: Ena is %d, dir %d", _state & (1 << _STEPPER_ENABLE_BIT), _state & (1 << _STEPPER_DIR_BIT));
            Serial.println(debug_buffer);
#endif
        } else {
#ifdef MYDEBUG
          sprintf(debug_buffer, "Stepper is off, encoder dir %d", dir);
          Serial.println(debug_buffer);
#endif
          if (dir) {
            if (dir > 0) {
              changeStepperDir<StepperDir::Forward>();
            } else {
              changeStepperDir<StepperDir::Backward>();
            }
#ifdef MYDEBUG
            sprintf(debug_buffer, "Stepper is off stepper dir %d state %d", _state & (1 << _STEPPER_DIR_BIT), _state);
            Serial.println(debug_buffer);
#endif
            toggleStepper();
            setState<StateOp::On>(_OLED_UPDATE_BIT);
#ifdef MYDEBUG
            sprintf(debug_buffer, "Off: Ena is %d dir %d state %d", _state & (1 << _STEPPER_ENABLE_BIT), _state & (1 << _STEPPER_DIR_BIT)), _state;
            Serial.println(debug_buffer);
#endif
          }
        }
      }
    }

    if (isState(_OLED_UPDATE_BIT) && millis() - _oledTimer >= 100) {
      _oledTimer = millis();  // сбросить таймер
      updateOLEDState();
      setState<StateOp::Off>(_OLED_UPDATE_BIT);
    }

  }

protected:
  inline uint32_t delayFromRPM() {
    return ((uint32_t)60 * 1000 * 1000) / _stepper_rpm / STEPS_PER_TURN / 2;  // 50% duty cycle
  }


  template<StateOp op>
  inline void setState(uint8_t bit) {
    switch (op)
    {
    case StateOp::On:
      _state |= (1 << bit);
      break;
    case StateOp::Off:
      _state &= ~(1 << bit);
      break;
    case StateOp::Toggle:
        _state ^= (1 << bit);  // инвертировать флаг
      break;
    
    default:
      break;
    }
  }

  inline bool isState(uint8_t bit) {
    return (_state & (1 << bit));
  } 

  template <StepperDir dir>
  void changeStepperDir() {
    switch (dir)
    {
      case StepperDir::Toggle:
        _state ^= (1 << _STEPPER_DIR_BIT);
        break;
      case StepperDir::Forward:
      _state &= ~(1 << _STEPPER_DIR_BIT);
        break;
      case StepperDir::Backward:
      _state |= (1 << _STEPPER_DIR_BIT);
        break;
      default:
        break;
    }
    digitalWrite(PIN_STEPPER_DIR, _state & (1 << _STEPPER_DIR_BIT));
  }

  void toggleStepper() {
    setState<StateOp::Toggle>(_STEPPER_ENABLE_BIT);
    digitalWrite(PIN_STEPPER_ENA, !(isState(_STEPPER_ENABLE_BIT))); // ENA has reversed logic.
    if (isState(_STEPPER_ENABLE_BIT)) {
      Timer2.restart();
    } else {
      Timer2.stop();
    }
  }

  inline int fastStep() {
    return _stepper_rpm >= 20 ? 5 : (_stepper_rpm > 10 ? 2 : 1);
  }

  void updateOLEDState() {
    char t[7];
    if (isState(_STEPPER_ENABLE_BIT)) {
      sprintf(t, "%d RPM", _stepper_rpm);
    } else {
      sprintf(t, " STOP ");
    }
    _oled.setScale(2);
    _oled.setCursorXY(8, 8);
    _oled.print("      ");

    _oled.setCursorXY(8, 8);
    _oled.print(t);

    _oled.setScale(1);
    _oled.setCursorXY(102, 8);
    int8_t arrow_dir = (isState(_STEPPER_DIR_BIT)) ? 1 : 0;
    for (int8_t i = 0; i < 7; i++) {
      uint8_t b = (isState(_STEPPER_ENABLE_BIT)) ? pgm_read_byte(&(_s_icons[arrow_dir][i])) : 0;
      _oled.drawByte(b);
    }
#ifdef MYDEBUG
    Serial.println("OLED updated");
#endif
  }

private:
  uint8_t _stepper_rpm = DEFAULT_RPM;

  uint32_t _oledTimer = 0;

  uint8_t _state = 0;

  static constexpr uint8_t _STEPPER_ENABLE_BIT = 0;
  static constexpr uint8_t _STEPPER_DIR_BIT = 1;
  static constexpr uint8_t _STEPPER_STATE_BIT = 2;
  static constexpr uint8_t _OLED_UPDATE_BIT = 3;
  static constexpr uint8_t _EEPROM_UPDATE_BIT = 4;

  EncButton _encoder;
  OLED _oled;

  Debouncer<300, 5> _debouncer;
};

RodRoller roller;

void setup() {
  // put your setup code here, to run once:

  roller.init();

#ifdef MYDEBUG
  Serial.begin(9600);
#endif
}

void loop() {
  // put your main code here, to run repeatedly:

  roller.tick();
}
