#ifndef FurryOwl_ByteState_h
#define FurryOwl_ByteState_h

#include <stdint.h>

class ByteState {
public:
    ByteState(): _state(0) {}
    ByteState(uint8_t state) : _state(state) {}
    ByteState(const ByteState& state) : _state(state._state) {}

    void on(uint8_t bit) {
        _state |= (1 << bit);
    }

    void off(uint8_t bit) {
        _state &= ~(1 << bit);
    }

    void toggle(uint8_t bit) {
        _state ^= (1 << bit); // инвертировать флаг
    }

    bool is(uint8_t bit) { 
        return (_state & (1 << bit)); 
    }

private:
  uint8_t _state = 0;
};

#endif