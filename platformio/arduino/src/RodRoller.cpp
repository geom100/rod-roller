#include <RodRoller.h>
#include <GyverTimers.h>

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


RodRoller::RodRoller()
    : _encoder(PIN_ENC_1, PIN_ENC_2, PIN_ENC_KEY, INPUT, INPUT_PULLUP, LOW) {}

void RodRoller::init() {
    pinMode(PIN_STEPPER_STEP, OUTPUT);
    pinMode(PIN_STEPPER_DIR, OUTPUT);
    pinMode(PIN_STEPPER_ENA, OUTPUT);
    pinMode(PIN_STEPPER_M0, OUTPUT);
    pinMode(PIN_STEPPER_M1, OUTPUT);
    pinMode(PIN_STEPPER_M2, OUTPUT);

    digitalWrite(PIN_STEPPER_ENA, !(_state.is(_STEPPER_ENABLE_BIT)));
    digitalWrite(PIN_STEPPER_DIR, _state.is(_STEPPER_DIR_BIT));

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
    Serial.begin(9600);
    Serial.println("Starting...");
#endif
}

void RodRoller::tick() {
  if (_encoder.tick()) {
    if (_encoder.turn() && _encoder.pressing()) {
      int newRpm =
          _stepperRPM + (_encoder.fast() ? fastStep() : 1) * _encoder.dir();
      _stepperRPM = constrain(newRpm, MIN_RPM, MAX_RPM);
      Timer2.setPeriod(delayFromRPM());
      _state.on(_OLED_UPDATE_BIT);
    } else if (_encoder.turn()) {
      int dir = _debouncer.event(_encoder.dir());
      if (_state.is(_STEPPER_ENABLE_BIT)) {
#ifdef MYDEBUG
        sprintf(debug_buffer, "Stepper is on, encoder dir %d", dir);
        Serial.println(debug_buffer);
#endif
        if (dir > 0) {
          changeStepperDir<StepperDir::Toggle>();
          _state.on(_OLED_UPDATE_BIT);
        } else if (dir < 0) {
          toggleStepper();
          _state.on(_OLED_UPDATE_BIT);
        }
#ifdef MYDEBUG
        sprintf(debug_buffer, "On: Ena is %d, dir %d",
                _state.is(_STEPPER_ENABLE_BIT),
                _state.is(_STEPPER_DIR_BIT));
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
          sprintf(debug_buffer, "Stepper is off stepper dir %d",
                  _state.is(_STEPPER_DIR_BIT));
          Serial.println(debug_buffer);
#endif
          toggleStepper();
          _state.on(_OLED_UPDATE_BIT);
#ifdef MYDEBUG
          sprintf(debug_buffer, "Off: Ena is %d dir %d state %d",
                  _state.is(_STEPPER_ENABLE_BIT),
                  _state.is(_STEPPER_DIR_BIT)),
              _state;
          Serial.println(debug_buffer);
#endif
        }
      }
    }
  }

  if (_state.is(_OLED_UPDATE_BIT) && millis() - _oledTimer >= 100) {
    _oledTimer = millis(); // сбросить таймер
    updateOLEDState();
    _state.off(_OLED_UPDATE_BIT);
  }
}

inline uint32_t RodRoller::delayFromRPM() {
  return ((uint32_t)60 * 1000 * 1000) / _stepperRPM / STEPS_PER_TURN /
         2; // 50% duty cycle
}

template <StepperDir dir> void RodRoller::changeStepperDir() {
  switch (dir) {
  case StepperDir::Toggle:
    _state.toggle(_STEPPER_DIR_BIT);
    break;
  case StepperDir::Forward:
    _state.off(_STEPPER_DIR_BIT);
    break;
  case StepperDir::Backward:
    _state.on(_STEPPER_DIR_BIT);
    break;
  default:
    break;
  }
  digitalWrite(PIN_STEPPER_DIR, _state.is(_STEPPER_DIR_BIT));
}

void RodRoller::toggleStepper() {
  _state.toggle(_STEPPER_ENABLE_BIT);
  digitalWrite(PIN_STEPPER_ENA,
               !(_state.is(_STEPPER_ENABLE_BIT))); // ENA has reversed logic.
  if (_state.is(_STEPPER_ENABLE_BIT)) {
    Timer2.restart();
  } else {
    Timer2.stop();
  }
}

inline int RodRoller::fastStep() {
  return _stepperRPM >= 20 ? 5 : (_stepperRPM > 10 ? 2 : 1);
}

void RodRoller::updateOLEDState() {
  char t[7];
  if (_state.is(_STEPPER_ENABLE_BIT)) {
    sprintf(t, "%d RPM", _stepperRPM);
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
  int8_t arrow_dir = (_state.is(_STEPPER_DIR_BIT)) ? 1 : 0;
  for (int8_t i = 0; i < 7; i++) {
    uint8_t b = (_state.is(_STEPPER_ENABLE_BIT))
                    ? pgm_read_byte(&(_s_icons[arrow_dir][i]))
                    : 0;
    _oled.drawByte(b);
  }
#ifdef MYDEBUG
  Serial.println("OLED updated");
#endif
}
