#include <RodRoller.h>
#include <GyverTimers.h>
#include <GyverIO.h>

// #define MYDEBUG 1

#ifdef MYDEBUG
char debug_buffer[128];
#endif

const static uint8_t PROGMEM _s_icons[][7] =
{
    {0x40, 0x20, 0x11, 0x09, 0x05, 0x03, 0x1F}, // Стрелка вверх 
    {0x7C, 0x60, 0x50, 0x48, 0x44, 0x02, 0x01} // Стрелка вниз 
};

void Stepper::init() {
    gio::mode(Pins::PIN_STEPPER_STEP, OUTPUT);
    gio::mode(Pins::PIN_STEPPER_DIR, OUTPUT);
    gio::mode(Pins::PIN_STEPPER_ENA, OUTPUT);
    gio::mode(Pins::PIN_STEPPER_M0, OUTPUT);
    gio::mode(Pins::PIN_STEPPER_M1, OUTPUT);
    gio::mode(Pins::PIN_STEPPER_M2, OUTPUT);

    // Pins are connected to driver switches on the board.
    gio::low(Pins::PIN_STEPPER_M0);
    gio::low(Pins::PIN_STEPPER_M1);
    gio::high(Pins::PIN_STEPPER_M2);

    Timer2.outputEnable(CHANNEL_B, TOGGLE_PIN);
    Timer2.setPeriod(delayFromRPM());
    Timer2.stop();
}

void Stepper::rpm(int8_t updown, bool fast) {
      // Intentionally unsafe! So we save a couple of processor tacts.
      // To check updown like this is much safer: 
      // int8_t signum = ((updown > 0) - (updown < 0));
      int newRpm =
          _stepperRPM + (fast ? fastStep() : 1) * updown;
      _stepperRPM = constrain(newRpm, MIN_RPM, MAX_RPM);
      Timer2.setPeriod(delayFromRPM());
}

void Stepper::dir(bool dir) {
  gio::write(Pins::PIN_STEPPER_DIR, dir);
}

void Stepper::ena(bool ena) {
  gio::write(Pins::PIN_STEPPER_ENA, !ena); // ENA has reversed logic.
  if (ena) {
    Timer2.restart();
  } else {
    Timer2.stop();
  }
}

inline uint32_t Stepper::delayFromRPM() {
  return ((uint32_t)60 * 1000 * 1000) / _stepperRPM / STEPS_PER_TURN /
         2; // 50% duty cycle
}

inline int Stepper::fastStep() {
  return _stepperRPM >= 20 ? 5 : (_stepperRPM > 10 ? 2 : 1);
}

void RodRoller::init() {
    _stepper.init();
    _stepper.dir(_state.is(_STEPPER_DIR_BIT));
    _stepper.ena(_state.is(_STEPPER_ENABLE_BIT));

    _encoder.setEncType(EB_STEP4_LOW);
    _encoder.setEncReverse(true);

    _oled.init();
    _oled.clear();

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
      _stepper.rpm(_encoder.dir(), _encoder.fast());
      _state.on(_OLED_UPDATE_BIT);
    } else if (_encoder.turn()) {
      int dir = _debouncer.event(_encoder.dir());
      if (_state.is(_STEPPER_ENABLE_BIT)) {
#ifdef MYDEBUG
        sprintf(debug_buffer, "Stepper is on, encoder dir %d", dir);
        Serial.println(debug_buffer);
#endif
        if (dir > 0) {
          _state.toggle(_STEPPER_DIR_BIT);
          _stepper.dir(_state.is(_STEPPER_DIR_BIT));
          _state.on(_OLED_UPDATE_BIT);
        } else if (dir < 0) {
          _state.toggle(_STEPPER_ENABLE_BIT);
          _stepper.ena(_state.is(_STEPPER_ENABLE_BIT));
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
            _state.off(_STEPPER_DIR_BIT);
          } else {
            _state.on(_STEPPER_DIR_BIT);
          }
          _stepper.dir(_state.is(_STEPPER_DIR_BIT));
#ifdef MYDEBUG
          sprintf(debug_buffer, "Stepper is off stepper dir %d",
                  _state.is(_STEPPER_DIR_BIT));
          Serial.println(debug_buffer);
#endif
          _state.toggle(_STEPPER_ENABLE_BIT);
          _stepper.ena(_state.is(_STEPPER_ENABLE_BIT));
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

void RodRoller::updateOLEDState() {
  char t[7];
  if (_state.is(_STEPPER_ENABLE_BIT)) {
    sprintf(t, "%d RPM", _stepper.currentRPM());
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
